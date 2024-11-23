#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "../include/entity.h"
#include "../include/route.h"
#include "../include/log.h"

// Define the VEHICLE_TAG for MPI message passing
#define VEHICLE_TAG 1

int getProcessRankForY(int y, int size);
void initializeLocalGrid(Map* map, Node** local_grid, int my_row_start, int my_row_end);
int getEdgeCount(Map* map, int x, int y);

int main(int argc, char *argv[]) {
    log_set_level(LOG_INFO);
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Determine grid partitioning
    int rows_per_process = GRID_HEIGHT / size;
    int extra_rows = GRID_HEIGHT % size;

    int my_row_start = rank * rows_per_process + (rank < extra_rows ? rank : extra_rows);
    int my_row_end = my_row_start + rows_per_process - 1;
    if (rank < extra_rows) {
        // Distribute the extra rows among the first 'extra_rows' processes
        my_row_end += 1;
    }

    int local_grid_height = my_row_end - my_row_start + 1;

    // Allocate the local grid dynamically
    Node** local_grid = (Node**)malloc(GRID_WIDTH * sizeof(Node*));
    for (int i = 0; i < GRID_WIDTH; i++) {
        local_grid[i] = (Node*)malloc(local_grid_height * sizeof(Node));
    }

    // Initialize map and local grid
    Map map;
    initMap(&map);

    // Initialize the local grid
    initializeLocalGrid(&map, local_grid, my_row_start, my_row_end);

    MPI_Barrier(MPI_COMM_WORLD);

    // Set random seed
    unsigned int seed;
    if (rank == 0) {
        seed = (unsigned int)time(NULL);
    }
    MPI_Bcast(&seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    srand(seed + rank);

    // Calculate vehicles per process
    const int base_vehicles = NUM_VEHICLES / size;
    const int extra = NUM_VEHICLES % size;
    const int local_vehicle_count = base_vehicles + (rank < extra ? 1 : 0);
    const int start_vehicle = rank * base_vehicles + (rank < extra ? rank : extra);

    // Create vehicles array
    Vehicle* local_vehicles = malloc(local_vehicle_count * sizeof(Vehicle));
    if (!local_vehicles) {
        log_error("Process %d: Failed to allocate memory for local vehicles\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Initialize vehicles
    for (int i = 0; i < local_vehicle_count; i++) {
        const int vehicle_id = start_vehicle + i;
        local_vehicles[i].id = vehicle_id;

        // Generate valid starting position within this process's partition
        do {
            local_vehicles[i].current.x = rand() % GRID_WIDTH;
            local_vehicles[i].current.y = my_row_start + (rand() % local_grid_height);
        } while (getEdgeCount(&map, local_vehicles[i].current.x, local_vehicles[i].current.y) == 0);

        // Generate valid destination (can be anywhere in the grid)
        do {
            local_vehicles[i].destination.x = rand() % GRID_WIDTH;
            local_vehicles[i].destination.y = rand() % GRID_HEIGHT;
        } while (getEdgeCount(&map, local_vehicles[i].destination.x, local_vehicles[i].destination.y) == 0 ||
                 (local_vehicles[i].destination.x == local_vehicles[i].current.x &&
                  local_vehicles[i].destination.y == local_vehicles[i].current.y));

        log_debug("Process %d: Vehicle %d starts at (%d, %d) and wants to reach (%d, %d)\n",
                  rank, vehicle_id, local_vehicles[i].current.x, local_vehicles[i].current.y,
                  local_vehicles[i].destination.x, local_vehicles[i].destination.y);

        // Enqueue the vehicle in the local grid
        int local_y = local_vehicles[i].current.y - my_row_start;
        enqueue(&local_grid[local_vehicles[i].current.x][local_y].q, local_vehicles[i]);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    const int MAX_STEP = 10000;
    int step = 0;
    int local_vehicle_remaining = local_vehicle_count;
    int total_vehicles_remaining;

    const double start = MPI_Wtime();

    while (step < MAX_STEP) {
        int moved = 0;

        // Process incoming vehicles from other processes
        MPI_Status status;
        int flag;
        while (1) {
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
            if (!flag) break;

            if (status.MPI_TAG == VEHICLE_TAG) {
                Vehicle incoming_vehicle;
                MPI_Recv(&incoming_vehicle, sizeof(Vehicle), MPI_BYTE, status.MPI_SOURCE, VEHICLE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Enqueue the vehicle in the local grid
                int local_y = incoming_vehicle.current.y - my_row_start;
                enqueue(&local_grid[incoming_vehicle.current.x][local_y].q, incoming_vehicle);
            }
        }

        // Process local vehicles
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < local_grid_height; j++) {
                Node* currentNode = &local_grid[i][j];

                if (isVehicleQueueEmpty(&currentNode->q)) {
                    continue;
                }

                Vehicle v = peek(&currentNode->q);

                // Skip if not our vehicle
                if (v.id < start_vehicle || v.id >= start_vehicle + local_vehicle_count) {
                    continue;
                }

                dequeue(&currentNode->q);

                int global_x = i;
                int global_y = my_row_start + j;

                // Check if arrived
                if (v.destination.x == global_x && v.destination.y == global_y) {
                    local_vehicle_remaining--;
                    log_debug("Process %d: Vehicle %d arrived at (%d, %d)\n",
                              rank, v.id, global_x, global_y);
                    continue;
                }

                // Compute next move
                RoutingResult newRoute = aStar(&map, global_x, global_y, v.destination.x, v.destination.y);

                if (newRoute.route.size > 1) {
                    Location nextNode = newRoute.route.locations[1];
                    v.current = nextNode;

                    int next_x = nextNode.x;
                    int next_y = nextNode.y;

                    // Determine which process owns the next position
                    int dest_rank = getProcessRankForY(next_y, size);

                    if (dest_rank == rank) {
                        // The next position is within the local grid
                        int local_next_y = next_y - my_row_start;
                        enqueue(&local_grid[next_x][local_next_y].q, v);
                    } else {
                        // Send the vehicle to the appropriate process
                        MPI_Send(&v, sizeof(Vehicle), MPI_BYTE, dest_rank, VEHICLE_TAG, MPI_COMM_WORLD);
                    }

                    moved = 1;
                } else {
                    log_debug("Process %d: Vehicle %d blocked at (%d, %d)\n", rank, v.id, global_x, global_y);
                    // Re-enqueue the vehicle in the current node
                    enqueue(&currentNode->q, v);
                }

                if (newRoute.route.locations) {
                    free(newRoute.route.locations);
                }
            }
        }

        // Synchronize remaining vehicles count
        MPI_Allreduce(&local_vehicle_remaining, &total_vehicles_remaining, 1,
                      MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        // Share movement information
        int global_moved;
        MPI_Allreduce(&moved, &global_moved, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

        if (global_moved == 0) {
            if (rank == 0) log_debug("No vehicle moved in step %d\n", step);
            if (step > MAX_STEP - 1000) break;
        }

        if (total_vehicles_remaining == 0) {
            break;
        }

        step++;
        MPI_Barrier(MPI_COMM_WORLD);
    }
    const double end = MPI_Wtime();
    if (rank == 0) {
        log_info("Average step time: %f\n", (end - start) / step);
    }

    // Final report
    if (rank == 0) {
        if (step >= MAX_STEP) {
            log_info("Max steps reached. %d vehicles remaining.\n", total_vehicles_remaining);
        } else {
            log_info("%d steps used for routing %d vehicles\n", step, NUM_VEHICLES);
        }
    }

    // Cleanup
    free(local_vehicles);
    for (int i = 0; i < GRID_WIDTH; i++) {
        free(local_grid[i]);
    }
    free(local_grid);
    MPI_Finalize();
    return 0;
}

// Function to determine which process owns a given y-coordinate
int getProcessRankForY(int y, int size) {
    int rows_per_process = GRID_HEIGHT / size;
    int extra_rows = GRID_HEIGHT % size;
    int rank = 0;

    int row_start = 0;
    for (int i = 0; i < size; i++) {
        int rows = rows_per_process + (i < extra_rows ? 1 : 0);
        int row_end = row_start + rows - 1;
        if (y >= row_start && y <= row_end) {
            rank = i;
            break;
        }
        row_start = row_end + 1;
    }
    return rank;
}

// Function to initialize the local grid
void initializeLocalGrid(Map* map, Node** local_grid, int my_row_start, int my_row_end) {
    int local_grid_height = my_row_end - my_row_start + 1;

    // Initialize local grid nodes
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int j = 0; j < local_grid_height; j++) {
            // Removed assignments to local_grid[x][j].x and .y since Node doesn't have these members
            local_grid[x][j].edgeCount = 0;
            local_grid[x][j].volume = 0;
            initVehicleQueue(&local_grid[x][j].q);
        }
    }

    // Set up edges in the local grid based on roads in the map
    for (int i = 0; i < map->numRoads; i++) {
        Road road = map->roads[i];
        int start_x = road.beginX;
        int start_y = road.beginY;
        int end_x = road.endX;
        int end_y = road.endY;

        // Adjust to ensure start <= end
        if (start_x > end_x) {
            int temp = start_x;
            start_x = end_x;
            end_x = temp;
        }
        if (start_y > end_y) {
            int temp = start_y;
            start_y = end_y;
            end_y = temp;
        }

        // Check if this road is within the local grid's y-range
        if (end_y < my_row_start || start_y > my_row_end) {
            continue; // Road is not in local grid
        }

        // Clamp road y-range to local grid
        int local_start_y = (start_y < my_row_start) ? my_row_start : start_y;
        int local_end_y = (end_y > my_row_end) ? my_row_end : end_y;

        // If the road is vertical
        if (start_x == end_x) {
            for (int y = local_start_y; y <= local_end_y; y++) {
                int local_y = y - my_row_start;
                if (y + 1 <= end_y && y + 1 <= my_row_end) {
                    addEdge(&local_grid[start_x][local_y], start_x, y + 1, 1);
                }
                if (y - 1 >= start_y && y - 1 >= my_row_start) {
                    addEdge(&local_grid[start_x][local_y], start_x, y - 1, 1);
                }
            }
        }

        // If the road is horizontal
        if (start_y == end_y) {
            // Check if the road's y-coordinate is within the local grid's y-range
            if (start_y >= my_row_start && start_y <= my_row_end) {
                int local_y = start_y - my_row_start;
                for (int x = start_x; x <= end_x; x++) {
                    if (x + 1 <= end_x) {
                        addEdge(&local_grid[x][local_y], x + 1, start_y, 1);
                    }
                    if (x - 1 >= start_x) {
                        addEdge(&local_grid[x][local_y], x - 1, start_y, 1);
                    }
                }
            }
        }
    }
}

// Function to get the edge count from the map at a given position
int getEdgeCount(Map* map, int x, int y) {
    // Check if (x, y) is on any road
    for (int i = 0; i < map->numRoads; i++) {
        Road road = map->roads[i];
        int start_x = road.beginX;
        int start_y = road.beginY;
        int end_x = road.endX;
        int end_y = road.endY;

        // Adjust to ensure start <= end
        if (start_x > end_x) {
            int temp = start_x;
            start_x = end_x;
            end_x = temp;
        }
        if (start_y > end_y) {
            int temp = start_y;
            start_y = end_y;
            end_y = temp;
        }

        if (start_x == end_x) {
            // Vertical road
            if (x == start_x && y >= start_y && y <= end_y) {
                return 1; // Node is on a road
            }
        } else if (start_y == end_y) {
            // Horizontal road
            if (y == start_y && x >= start_x && x <= end_x) {
                return 1; // Node is on a road
            }
        }
    }
    return 0; // Node is not on a road
}
