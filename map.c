// map.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <_string.h>

#include "entity.h"
#include "route.h"

#define NUM_VEHICLES 1000

Node grid[GRID_WIDTH][GRID_HEIGHT];
Vehicle vehicles[NUM_VEHICLES];
int activeVehicles[NUM_VEHICLES]; // Queue to track active vehicles
int vehicleCount = NUM_VEHICLES;

// Function to create vehicles (unchanged)
void initializeVehicles(Map *map) {
    srand(time(NULL));
    for (int i = 0; i < NUM_VEHICLES; i++) {
        vehicles[i].id = i;

        // Ensure vehicles are placed randomly on the grid within valid road areas
        do {
            vehicles[i].current.x = rand() % GRID_WIDTH;
            vehicles[i].current.y = rand() % GRID_HEIGHT;
        } while (grid[vehicles[i].current.x][vehicles[i].current.y].edgeCount == 0 ||
                 grid[vehicles[i].current.x][vehicles[i].current.y].volume > grid[vehicles[i].current.x][vehicles[i].current.y].capacity);

        // Ensure destination is also randomly chosen but different from the starting point
        do {
            vehicles[i].destination.x = rand() % GRID_WIDTH;
            vehicles[i].destination.y = rand() % GRID_HEIGHT;
        } while ((vehicles[i].destination.x == vehicles[i].current.x && vehicles[i].destination.y == vehicles[i].current.y) ||
                 grid[vehicles[i].destination.x][vehicles[i].destination.y].edgeCount == 0);

        // Initialize active vehicle queue
        vehicles[i].activate = 1;

        // Increase the volume of the node
        grid[vehicles[i].current.x][vehicles[i].current.y].volume++;

        // Print initial vehicle information
        printf("Vehicle %d starts at (%d, %d) and wants to reach (%d, %d)\n",
               vehicles[i].id, vehicles[i].current.x, vehicles[i].current.y,
               vehicles[i].destination.x, vehicles[i].destination.y);
    }
}

// Function to add edges between nodes (unchanged)
void addEdge(Node *node, int x, int y, int weight) {
    if (node->edgeCount < 4) {
        node->edges[node->edgeCount] = (Edge){x, y, weight};
        node->edgeCount++;
    }
}

// Function to initialize the grid (unchanged)
void initializeGrid(Map *map) {
    // Initialize grid with empty nodes
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].edgeCount = 0;
            grid[i][j].volume = 0; // Initialize all nodes as unoccupied
            grid[i][j].capacity = 10;
        }
    }

    // Set up edges in the grid based on roads
    for (int i = 0; i < map->numRoads; i++) {
        int x_start = map->roads[i].beginX;
        int y_start = map->roads[i].beginY;
        int x_end = map->roads[i].endX;
        int y_end = map->roads[i].endY;

        // If the road is vertical
        if (x_start == x_end) {
            for (int y = y_start; y <= y_end; y++) {
                // Add bidirectional edges between consecutive nodes along the road
                if (y + 1 <= y_end) {
                    addEdge(&grid[x_start][y], x_start, y + 1, 1); // Downward edge
                    addEdge(&grid[x_start][y + 1], x_start, y, 1); // Upward edge (bidirectional)
                }
            }
        }

        // If the road is horizontal
        if (y_start == y_end) {
            for (int x = x_start; x <= x_end; x++) {
                // Add bidirectional edges between consecutive nodes along the road
                if (x + 1 <= x_end) {
                    addEdge(&grid[x][y_start], x + 1, y_start, 1); // Rightward edge
                    addEdge(&grid[x + 1][y_start], x, y_start, 1); // Leftward edge (bidirectional)
                }
            }
        }
    }
}

int main() {
    Map map;
    FILE *logFile = fopen("vehicle_log.txt", "w");
    if (logFile == NULL) {
        printf("Error opening log file!\n");
        return 1;
    }

    initMap(&map);
    printMap(&map);

    // Initialize grid
    initializeGrid(&map);

    // Initialize vehicles
    initializeVehicles(&map);

    // Simulation loop
    int step = 0;
    int maxSteps = 1000; // Maximum number of steps to prevent infinite loops
    int waitingTime[NUM_VEHICLES] = {0}; // Array to keep track of waiting time for each vehicle
    int DEADLOCK_THRESHOLD = 100; // Threshold for deadlock detection
    while (vehicleCount > 0 && step < maxSteps) {
        int movedInStep = 0;
        // A snapshot for every step
        Node tempGrid[GRID_WIDTH][GRID_HEIGHT];
        memcpy(tempGrid, grid, sizeof(grid));

        for (int i = 0; i < NUM_VEHICLES; i++) {
            Vehicle *v = &vehicles[i];
            if (v->activate == 0) {
                continue;
            }
            // Check if the vehicle has reached its destination
            if (v->current.x == v->destination.x && v->current.y == v->destination.y) {
                fprintf(logFile, "Step %d: Vehicle %d has reached its destination at (%d, %d)\n",
                        step, v->id, v->destination.x, v->destination.y);
                v->activate = 0;
                vehicleCount--;
                tempGrid[v->current.x][v->current.y].volume--;
                continue;
            }
            // Find the shortest path using A*
            PathResult pathResult = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y,
                                          GRID_WIDTH, GRID_HEIGHT, -1, -1);

            if (pathResult.path_length != -1) {
                int next_x = pathResult.next_x;
                int next_y = pathResult.next_y;

                if (grid[next_x][next_y].volume < grid[next_x][next_y].capacity) {
                    // Move the vehicle
                    tempGrid[v->current.x][v->current.y].volume--;
                    v->current.x = next_x;
                    v->current.y = next_y;
                    tempGrid[next_x][next_y].volume++;
                    movedInStep = 1;
                    waitingTime[i] = 0; // Reset waiting time since the vehicle moved
                } else {
                    // The next node is occupied
                    waitingTime[i]++;

                    // Check if waiting time exceeds DEADLOCK_THRESHOLD
                    if (waitingTime[i] > DEADLOCK_THRESHOLD) {
                        // Try to find an alternative path that avoids the blocked node
                        PathResult newPathResult = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y,
                                                         GRID_WIDTH, GRID_HEIGHT, next_x, next_y);

                        if (newPathResult.path_length != -1 && !(newPathResult.next_x == next_x && newPathResult.next_y == next_y)) {
                            // Found an alternative path
                            next_x = newPathResult.next_x;
                            next_y = newPathResult.next_y;

                            if (grid[next_x][next_y].volume < grid[next_x][next_y].capacity) {
                                // Move the vehicle
                                tempGrid[v->current.x][v->current.y].volume--;
                                v->current.x = next_x;
                                v->current.y = next_y;
                                tempGrid[next_x][next_y].volume++;
                                movedInStep = 1;
                                waitingTime[i] = 0; // Reset waiting time
                            } else {
                                // Cannot move, wait
                                fprintf(logFile, "Step %d: Vehicle %d cannot move to (%d, %d), occupied after rerouting\n",
                                        step, v->id, next_x, next_y);
                            }
                        } else {
                            // Cannot find alternative path, wait
                            fprintf(logFile, "Step %d: Vehicle %d cannot find alternative path after rerouting\n", step, v->id);
                        }
                    } else {
                        // Wait and try again in the next time step
                        fprintf(logFile, "Step %d: Vehicle %d is waiting at (%d, %d)\n",
                                step, v->id, v->current.x, v->current.y);
                    }
                }
            } else {
                // No path found, possibly because all routes are blocked
                fprintf(logFile, "Step %d: Vehicle %d could not find a path to (%d, %d)\n",
                        step, v->id, v->destination.x, v->destination.y);
                waitingTime[i]++;
            }
        }
        static int noMoveCounter = 0;
        if (movedInStep == 0) {
            fprintf(logFile, "No vehicles moved in time step %d. Continuing simulation.\n", step);
            // If no vehicles have moved for several steps, we might decide to end the simulation
            
            noMoveCounter++;
            if (noMoveCounter > DEADLOCK_THRESHOLD * 2) {
                fprintf(logFile, "No vehicles have moved for %d consecutive steps. Ending simulation.\n", noMoveCounter);
                break;
            }
        } else {
            noMoveCounter = 0; // Reset the counter since at least one vehicle moved
        }
        memcpy(grid, tempGrid, sizeof(grid));
        step++;
    }

    if (step >= maxSteps) {
        fprintf(logFile, "Maximum number of steps reached. Ending simulation.\n");
    }

    fclose(logFile);
    return 0;
}
