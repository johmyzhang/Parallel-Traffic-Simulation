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

    Vehicle *v = &vehicles[0];
    int sp = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);
    printf("With volume: sp = %d, md = %d\n", sp, heuristic(v->current.x, v->current.y, v->destination.x, v->destination.y));
    initializeGrid(&map);
    sp = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);
    printf("Without volume: sp = %d, md = %d\n", sp, heuristic(v->current.x, v->current.y, v->destination.x, v->destination.y));


    return 0;
}
