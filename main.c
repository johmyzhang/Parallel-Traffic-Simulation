#include <assert.h>
#include <stdlib.h>
#include </usr/local/Cellar/open-mpi/5.0.6/include/mpi.h>
#include "entity.h"
#include "route.h"
#include "log.h"
#include "mpiutil.h"
#include <unistd.h>
#include <string.h>

// Grid is shared?
// Grid itself do not need to be shared -> sharing node queue is unnecessary because queue is only needed for local ops
// However queue size needs to be shared since routing cost needs it.
// Thus, a new MPI_Datatype which contains sizes of q of every node is needed.
Node grid[GRID_WIDTH][GRID_HEIGHT];

int main(int argc, char *argv[]) {

    log_set_level(LOG_DEBUG);

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Map map;
    initMap(&map);
    // Map is no longer needed after grid is initialized.
    initializeGrid(&map);

    Vehicle *vehicles = (Vehicle *)malloc(sizeof(Vehicle) * NUM_VEHICLES);

    // Calculate communication buffer size
    int bufferSize = NUM_VEHICLES * 5 * sizeof(int);
    int *buffer = (int*) malloc(bufferSize*sizeof(int));
    if (rank == 0) {
        // Vehicle array is shared using buffer. By this time vehicles are not in the grid.
        initializeVehicles(vehicles);
        serializeVehicleArray(vehicles, buffer, NUM_VEHICLES);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Bcast(buffer, bufferSize, MPI_BYTE, 0, MPI_COMM_WORLD);
    // int k = 0;
    // while(!k)
    //     sleep(5);
    deserializeVehicles(vehicles, buffer, NUM_VEHICLES);
    free(buffer);
    buffer = NULL;

    // Placing vehicles
    for (int i = 0; i < NUM_VEHICLES; i++) {
        int x = vehicles[i].current.x;
        int y = vehicles[i].current.y;
        // Is node queue in every process same after placing vehicles?
        // Yes because the array remains the same after deserializing.
        enqueue(&grid[x][y].q, vehicles[i]);
        grid[x][y].volume = grid[x][y].q.size;
    }

    // Assign areas
    int areaWidth = GRID_WIDTH/size;
    int startAt = rank * areaWidth;
    int MAX_STEP = 1000;
    int step = 0;
    int vehicleCount = NUM_VEHICLES;
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = startAt; j < startAt + areaWidth; j++) {
            if(grid[i][j].volume != 0 && !(peek(&grid[i][j].q).current.x == i && peek(&grid[i][j].q).current.y == j)) {
                printf("Pre check failed!");
                assert(peek(&grid[i][j].q).current.x == i && peek(&grid[i][j].q).current.y == j);
            }
        }
    }
    int k = 0;
    while (!k)
        sleep(5);
    while (step < MAX_STEP && vehicleCount > 0) {
        // int hasVehicle = 0;
        int numArrived = 0;
        int totalArrived = 0;
        int *vidToBeSent = (int*) malloc(sizeof(int) * areaWidth * size);
        for (int i = 0; i < areaWidth * size; i++) {
            vidToBeSent[i] = -1;
        }
        int *numVehiclesToBeSent = (int*) malloc(sizeof(int) * size);
         int vehicleBufferSize = 5 * sizeof(int);
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = startAt; j < startAt + areaWidth; j++) {
                Node *currentNode = &grid[i][j];
                // No vehicle present or invalid node
                if (isVehicleQueueEmpty(&currentNode->q)) {
                    continue;
                }
                // hasVehicle = 1;

                Vehicle *v = &vehicles[peek(&currentNode->q).id];
                // Arrived
                if (v->destination.x == i && v->destination.y == j) {
                    numArrived++;
                    log_debug("Vehicle %d arrived at (%d, %d)\n", v->id, i, j);
                    dequeue(&currentNode->q);
                    grid[i][j].volume = currentNode->q.size;
                    continue;
                }

                RoutingResult newRoute = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);

                // Location newRoute.route.locations[2] = {};
                // TODO: Check x or y
                // Pass the vehicle to other process if destination is not within the area.
                v->current.x = newRoute.route.locations[1].x;
                v->current.y = newRoute.route.locations[1].y;
                dequeue(&currentNode->q);
                grid[i][j].volume = currentNode->q.size;
                if (newRoute.route.locations[1].y > startAt + areaWidth || newRoute.route.locations[1].y < startAt) {
                    int destProcess = newRoute.route.locations[1].y/areaWidth;
                    vidToBeSent[destProcess * areaWidth + numVehiclesToBeSent[destProcess]] = v->id;
                    numVehiclesToBeSent[destProcess]++;

                } else {
                    enqueue(&grid[newRoute.route.locations[1].x][newRoute.route.locations[1].y].q, *v);
                    grid[newRoute.route.locations[1].x][newRoute.route.locations[1].y].volume = grid[newRoute.route.locations[1].x][newRoute.route.locations[1].y].q.size;
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        // Sync crossing border vehicles
        int *recvLength = (int*)malloc(sizeof(int) * size);
        MPI_Alltoall(numVehiclesToBeSent, 1, MPI_INT, recvLength, 1, MPI_INT, MPI_COMM_WORLD);

        int totalRecvLength = 0;
        for (int i = 0; i < size; i++) {
            totalRecvLength += recvLength[i];
        }

        Vehicle *recvVehicles = (Vehicle *) malloc(totalRecvLength * vehicleBufferSize);
        Vehicle tempVehicle[totalRecvLength];
        for (int i = 0; i < size; i++) {
            if (rank != i) {
                if (numVehiclesToBeSent[i] == 0) {
                    continue;
                }
                Vehicle *vehiclesToBeSent = (Vehicle *) malloc(sizeof(Vehicle) * numVehiclesToBeSent[i]);
                for (int j = 0; j < numVehiclesToBeSent[i]; j++) {
                    vehiclesToBeSent[j].id = vehicles[vidToBeSent[i * areaWidth + j]].id;
                    vehiclesToBeSent[j].current.x = vehicles[vidToBeSent[i * areaWidth + j]].current.x;
                    vehiclesToBeSent[j].current.y = vehicles[vidToBeSent[i * areaWidth + j]].current.y;
                    vehiclesToBeSent[j].destination.x = vehicles[vidToBeSent[i * areaWidth + j]].destination.x;
                    vehiclesToBeSent[j].destination.y = vehicles[vidToBeSent[i * areaWidth + j]].destination.y;
                }

                int *syncBuffer = (int*) malloc(vehicleBufferSize * numVehiclesToBeSent[i] * sizeof(int));
                serializeVehicleArray(vehiclesToBeSent, syncBuffer, numVehiclesToBeSent[i]);
                MPI_Send(syncBuffer, vehicleBufferSize * numVehiclesToBeSent[i], MPI_INT, i, 0, MPI_COMM_WORLD);
                free(syncBuffer);
                free(vehiclesToBeSent);
                syncBuffer = NULL;
                vehiclesToBeSent = NULL;
            } else {
                int received = 0;
                for (int j = 0; j < size; j++) {
                    if (j == i || recvLength[j] == 0) {
                        continue;
                    }
                    int *syncBuffer = (int*) malloc(vehicleBufferSize * recvLength[j] * sizeof(int));
                    MPI_Recv(syncBuffer, vehicleBufferSize * recvLength[j], MPI_INT, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    deserializeVehicles(recvVehicles, syncBuffer, recvLength[j]);
                    for (int k = 0; k < recvLength[j]; k++) {
                        tempVehicle[received] = recvVehicles[k];
                        received++;
                    }
                    free(recvVehicles);
                    free(syncBuffer);
                    recvVehicles = NULL;
                    syncBuffer = NULL;
                }
            }
        }
        free(vidToBeSent);
        free(numVehiclesToBeSent);
        vidToBeSent = NULL;
        numVehiclesToBeSent = NULL;

        for (int i = 0; i < totalRecvLength; i++) {
            Vehicle *v = &tempVehicle[i];
            assert(grid[v->current.x][v->current.y].edgeCount != 0);
            enqueue(&grid[v->current.x][v->current.y].q, *v);
            grid[v->current.x][v->current.y].volume = grid[v->current.x][v->current.y].q.size;
        }

        // Sync total moved vehicles
        MPI_Allreduce(&numArrived, &totalArrived, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        vehicleCount -= totalArrived;

        // Sync node volume
        int localGridSize = GRID_HEIGHT * areaWidth;
        int *gridSnapShot = (int*) malloc(localGridSize * sizeof(int));
        int *recvBuffer = (int*) malloc(GRID_HEIGHT * GRID_WIDTH * sizeof(int));

        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = startAt; j < startAt + areaWidth; j++) {
                gridSnapShot[i * GRID_HEIGHT + j] = grid[i][j].volume;
            }
        }

        MPI_Allgather(gridSnapShot, localGridSize, MPI_INT, recvBuffer, localGridSize, MPI_INT, MPI_COMM_WORLD);

        // update grid
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                grid[i][j].volume = recvBuffer[i * GRID_WIDTH + j];
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            log_debug("Step %d -- Vehicles remain: %d", step, vehicleCount);
        }
        step++;

        free(gridSnapShot);
        free(recvBuffer);
        gridSnapShot = NULL;
        recvBuffer = NULL;
    }
    if (vehicleCount > 0) {
        log_debug("Max number of steps reached.");
    } else {
        log_debug("%d steps used for routing %d vehicles", step, NUM_VEHICLES);
    }
    free(vehicles);
    vehicles = NULL;
    MPI_Finalize();
}
