#include <stdio.h>
#include <stdlib.h>
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
    Vehicle *v = &vehicles[5];
    RoutingResult result = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);
    printf("Start: (%d, %d)\n", v->current.x, v->current.y);
    printf("Destination: (%d, %d)\n", v->destination.x, v->destination.y);
    for (int i = 0; i < result.pathTaken; i++) {
        printf("(%d, %d) ", result.route.locations[i].x, result.route.locations[i].y);
    }
    const int MAX_STEP = 1000;
    int step = 0;
    int vehicleCount = NUM_VEHICLES;
    while (step < MAX_STEP && vehicleCount > 0) {
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                Node *current = &grid[i][j];

            }
        }
        step++;
    }
    return 0;
}
