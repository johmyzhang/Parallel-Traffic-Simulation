#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>
#include "entity.h"
#include "route.h"

Node grid[GRID_WIDTH][GRID_HEIGHT];

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Initialize map and grid
    Map map;
    initMap(&map);
    initializeGrid(&map);
    MPI_Barrier(MPI_COMM_WORLD);

    // Set random seed
    unsigned int seed;
    if (rank == 0) {
        seed = (unsigned int)time(NULL);
    }
    MPI_Bcast(&seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    srand(seed + rank);

    // Calculate vehicles per process
    int base_vehicles = NUM_VEHICLES / size;
    int extra = NUM_VEHICLES % size;
    int local_vehicle_count = base_vehicles + (rank < extra ? 1 : 0);
    int start_vehicle = rank * base_vehicles + (rank < extra ? rank : extra);

    // Create vehicles array
    Vehicle* local_vehicles = malloc(local_vehicle_count * sizeof(Vehicle));
    if (!local_vehicles) {
        fprintf(stderr, "Process %d: Failed to allocate memory for local vehicles\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Initialize vehicles on all processes
    for (int i = 0; i < local_vehicle_count; i++) {
        int vehicle_id = start_vehicle + i;
        local_vehicles[i].id = vehicle_id;
        
        // Generate valid starting position
        do {
            local_vehicles[i].current.x = rand() % GRID_WIDTH;
            local_vehicles[i].current.y = rand() % GRID_HEIGHT;
        } while (grid[local_vehicles[i].current.x][local_vehicles[i].current.y].edgeCount == 0);

        // Generate valid destination
        do {
            local_vehicles[i].destination.x = rand() % GRID_WIDTH;
            local_vehicles[i].destination.y = rand() % GRID_HEIGHT;
        } while (grid[local_vehicles[i].destination.x][local_vehicles[i].destination.y].edgeCount == 0 ||
                (local_vehicles[i].destination.x == local_vehicles[i].current.x &&
                 local_vehicles[i].destination.y == local_vehicles[i].current.y));

        printf("Process %d: Vehicle %d starts at (%d, %d) and wants to reach (%d, %d)\n",
               rank, vehicle_id, local_vehicles[i].current.x, local_vehicles[i].current.y,
               local_vehicles[i].destination.x, local_vehicles[i].destination.y);
    }

    // Initialize queues
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            initVehicleQueue(&grid[i][j].q);
        }
    }

    // Place initial vehicles
    for (int i = 0; i < local_vehicle_count; i++) {
        Vehicle* v = &local_vehicles[i];
        assert(v->current.x >= 0 && v->current.x < GRID_WIDTH);
        assert(v->current.y >= 0 && v->current.y < GRID_HEIGHT);
        enqueue(&grid[v->current.x][v->current.y].q, *v);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    const int MAX_STEP = 10000;
    int step = 0;
    int local_vehicle_remaining = local_vehicle_count;
    int total_vehicles_remaining;
    
    while (step < MAX_STEP) {
        int moved = 0;
        
        // Process local vehicles
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                Node* currentNode = &grid[i][j];
                
                if (isVehicleQueueEmpty(&currentNode->q)) {
                    continue;
                }

                Vehicle v = peek(&currentNode->q);
                
                // Skip if not our vehicle
                if (v.id < start_vehicle || v.id >= start_vehicle + local_vehicle_count) {
                    continue;
                }

                dequeue(&currentNode->q);

                // Check if arrived
                if (v.destination.x == i && v.destination.y == j) {
                    local_vehicle_remaining--;
                    printf("Process %d: Vehicle %d arrived at (%d, %d)\n", 
                           rank, v.id, i, j);
                    continue;
                }

                // Compute next move
                RoutingResult newRoute = aStar(i, j, v.destination.x, v.destination.y);
                
                if (newRoute.route.size > 1) {
                    Location nextNode = newRoute.route.locations[1];
                    v.current = nextNode;
                    
                    if (nextNode.x >= 0 && nextNode.x < GRID_WIDTH && 
                        nextNode.y >= 0 && nextNode.y < GRID_HEIGHT) {
                        enqueue(&grid[nextNode.x][nextNode.y].q, v);
                        moved = 1;
                    } else {
                        enqueue(&currentNode->q, v);
                    }
                } else {
                    printf("Process %d: Vehicle %d blocked at (%d, %d)\n", 
                           rank, v.id, i, j);
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
            if (rank == 0) printf("No vehicle moved in step %d\n", step);
            if (step > MAX_STEP - 1000) break;
        }

        if (total_vehicles_remaining == 0) {
            break;
        }

        step++;
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // Final report
    if (rank == 0) {
        if (step >= MAX_STEP) {
            printf("Max steps reached. %d vehicles remaining.\n", total_vehicles_remaining);
        } else {
            printf("%d steps used for routing %d vehicles\n", step, NUM_VEHICLES);
        }
    }

    // Cleanup
    free(local_vehicles);
    MPI_Finalize();
    return 0;
}