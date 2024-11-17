#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "entity.h"
#include "route.h"

#define NUM_VEHICLES 1000

Node grid[GRID_WIDTH][GRID_HEIGHT];
Vehicle vehicles[NUM_VEHICLES];

// Function to create vehicles
void initializeVehicles(Map *map) {
    srand(time(NULL));
    for (int i = 0; i < NUM_VEHICLES; i++) {
        vehicles[i].id = i;

        // Ensure vehicles are placed randomly on the grid within valid road areas
        do {
            vehicles[i].current.x = rand() % GRID_WIDTH;
            vehicles[i].current.y = rand() % GRID_HEIGHT;
        } while (grid[vehicles[i].current.x][vehicles[i].current.y].edgeCount == 0);

        // Ensure destination is also randomly chosen but different from the starting point
        do {
            vehicles[i].destination.x = rand() % GRID_WIDTH;
            vehicles[i].destination.y = rand() % GRID_HEIGHT;
        } while ((vehicles[i].destination.x == vehicles[i].current.x && vehicles[i].destination.y == vehicles[i].current.y) ||
                 grid[vehicles[i].destination.x][vehicles[i].destination.y].edgeCount == 0);

        // Print initial vehicle information
        printf("Vehicle %d starts at (%d, %d) and wants to reach (%d, %d)\n",
               vehicles[i].id, vehicles[i].current.x, vehicles[i].current.y,
               vehicles[i].destination.x, vehicles[i].destination.y);
    }
}

// Revised function to add edges between nodes
void addEdge(Node *node, int x, int y, int weight) {
    if (node->edgeCount < 4) {
        node->edges[node->edgeCount] = (Edge){x, y, weight};
        node->edgeCount++;
    }
}

void initializeGrid(Map *map) {
    // Initialize grid with empty nodes
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].edgeCount = 0;
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
                if (y + 1 <= y_end) {
                    addEdge(&grid[x_start][y], x_start, y + 1, 1); // Downward edge
                }
                if (y - 1 >= y_start) {
                    addEdge(&grid[x_start][y], x_start, y - 1, 1); // Upward edge
                }
            }
        }

        // If the road is horizontal
        if (y_start == y_end) {
            for (int x = x_start; x <= x_end; x++) {
                if (x + 1 <= x_end) {
                    addEdge(&grid[x][y_start], x + 1, y_start, 1); // Rightward edge
                }
                if (x - 1 >= x_start) {
                    addEdge(&grid[x][y_start], x - 1, y_start, 1); // Leftward edge
                }
            }
        }
    }
}

int main() {
    Map map;

    initMap(&map);

    printMap(&map);

    // Initialize grid
    initializeGrid(&map);

    // Initialize vehicles
    initializeVehicles(&map);

    // int sp = aStar(32, 97, 33, 33, 100, 100);
    // printf("%d\n", sp);
    //
    // Simulation loop
    int step = 0;
    int vehicleCount = NUM_VEHICLES;
    while (vehicleCount > 0) {
        printf("\nTime step %d\n", step);

        for (int i = 0; i < NUM_VEHICLES; i++) {
            Vehicle *v = &vehicles[i];
            if (v->current.x == v->destination.x && v->current.y == v->destination.y) {
                // Reach destination
                // printf("Vehicle %d has reached its destination (%d, %d)\n",
                //        v->id, v->destination.x, v->destination.y);

                continue;
            }

            // Find the shortest path using A*
            int shortest_path_length = aStar(v->current.x, v->current.y,
                                           v->destination.x, v->destination.y,
                                           GRID_WIDTH, GRID_HEIGHT);

            // Move the vehicle if a path is found
            if (shortest_path_length != -1 && grid[v->current.x][v->current.y].edgeCount > 0) {
                // Attempt to follow the shortest path by selecting the edge that moves closer to the destination
                for (int j = 0; j < grid[v->current.x][v->current.y].edgeCount; j++) {
                    Edge next_edge = grid[v->current.x][v->current.y].edges[j];
                    if ((abs(next_edge.x - v->destination.x) + abs(next_edge.y - v->destination.y)) <
                        (abs(v->current.x - v->destination.x) + abs(v->current.y - v->destination.y))) {
                        v->current.x = next_edge.x;
                        v->current.y = next_edge.y;
                        printf("Vehicle %d moved to (%d, %d)\n", v->id, v->current.x, v->current.y);
                        break;
                    }
                }
            } else {
                printf("Vehicle %d could not find a path to (%d, %d)\n",
                       v->id, v->destination.x, v->destination.y);
            }
        }
    }

    return 0;
}
