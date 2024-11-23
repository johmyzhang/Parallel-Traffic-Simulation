#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "entity.h"
#include "route.h"
#include "log.h"

Node grid[GRID_WIDTH][GRID_HEIGHT];

int main() {
    log_set_level(LOG_INFO);
    Map map;
    initMap(&map);

    initializeGrid(&map);

    Vehicle vehicles[NUM_VEHICLES];

    // Initialize vehicles
    initializeVehicles(vehicles);

    const int MAX_STEP = 10000;
    int step = 0;
    int vehicleCount = NUM_VEHICLES;
    struct timespec start, end;
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) { perror( "clock gettime" );}
    while (step < MAX_STEP && vehicleCount > 0) {
        int moved = 0;
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                Node *currentNode = &grid[i][j];
                // No vehicle present or invalid node
                if (isVehicleQueueEmpty(&currentNode->q)) {
                    continue;
                }

                Vehicle *v = &vehicles[dequeue(&currentNode->q)];
                // Arrived
                if (v->destination.x == i && v->destination.y == j) {
                    vehicleCount--;
                    log_debug("Vehicle %d arrived at (%d, %d)\n", v->id, i, j);
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
        if (moved == 0) log_debug("No vehicle moved in step %d\n", step);
        step++;
    }
    if( clock_gettime( CLOCK_REALTIME, &end) == -1 ) { perror( "clock gettime" );}
    const double time = (end.tv_sec - start.tv_sec)+ (end.tv_nsec - start.tv_nsec)/1e9;
    log_info("Average step time: %f", time/step);
    if (vehicleCount > 0) {
        log_debug("Max number of steps reached.");
    } else {
        log_debug("%d steps used for routing %d vehicles", step, NUM_VEHICLES);
    }
    return 0;
}
