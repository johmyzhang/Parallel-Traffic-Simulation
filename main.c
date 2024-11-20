#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.h"
#include "route.h"

Node grid[GRID_WIDTH][GRID_HEIGHT];

int main() {

    Map map;
    initMap(&map);

    initializeGrid(&map);

    Vehicle vehicles[NUM_VEHICLES];

    // Initialize vehicles
    initializeVehicles(vehicles);
    // Vehicle *v = &vehicles[5];
    // RoutingResult result = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);
    // printf("Start: (%d, %d)\n", v->current.x, v->current.y);
    // printf("Destination: (%d, %d)\n", v->destination.x, v->destination.y);
    // for (int i = 0; i < result.pathTaken; i++) {
    //     printf("(%d, %d) ", result.route.locations[i].x, result.route.locations[i].y);
    // }
    const int MAX_STEP = 10000;
    int step = 0;
    int vehicleCount = NUM_VEHICLES;
    while (step < MAX_STEP && vehicleCount > 0) {
        int moved = 0;
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                Node *currentNode = &grid[i][j];
                // No vehicle present or invalid node
                //assert(!(currentNode->q.size == 0 && isVehicleQueueEmpty(&currentNode->q)));
                if (isVehicleQueueEmpty(&currentNode->q)) {
                    continue;
                }

                Vehicle *v = &vehicles[dequeue(&currentNode->q)];
                // Arrived
                if (v->destination.x == i && v->destination.y == j) {
                    vehicleCount--;
                    printf("Vehicle %d arrived at (%d, %d)\n", v->id, i, j);
                    continue;
                }

                assert(v->current.x == i && v->current.y == j);
                const RoutingResult newRoute = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);
                const Location nextNode = newRoute.route.locations[1];
                // Move along
                v->current.x = nextNode.x;
                v->current.y = nextNode.y;
                enqueue(&grid[v->current.x][v->current.y].q, *v);
                moved = 1;
            }
        }
        if (moved == 0) printf("No vehicle moved in step %d\n", step);
        step++;
    }
    if (vehicleCount > 0) {
        printf("Max number of steps reached.");
    } else {
        printf("%d steps used for routing %d vehicles", step, NUM_VEHICLES);
    }
    return 0;
}
