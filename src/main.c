#include <assert.h>
#include <stdlib.h>
#include <mpi.h>
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

    log_set_level(LOG_INFO);

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Map map;
    initMap(&map);
    // Map is no longer needed after grid is initialized.
    initializeGrid(&map);
    int areaWidth = GRID_WIDTH/size;
    int startAt = rank * areaWidth;
    Vehicle *vehicles = (Vehicle *)malloc(sizeof(Vehicle) * NUM_VEHICLES);

    // Calculate communication buffer size
    int bufferSize = NUM_VEHICLES * 5 * sizeof(int);
    int *buffer = (int*) malloc(bufferSize*sizeof(int));
    if (rank == 0) {
        // Vehicle array is shared using buffer. By this time vehicles are not in the grid.
        initializeVehicles(vehicles, startAt, areaWidth);
        serializeVehicleArray(vehicles, buffer, NUM_VEHICLES);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Bcast(buffer, bufferSize, MPI_BYTE, 0, MPI_COMM_WORLD);
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
    int MAX_STEP = 200;
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
    double start;
    if (rank == 0) start = MPI_Wtime();
    while (step < MAX_STEP && vehicleCount > 0) {
        int hasVehicle = 0;
        int numArrived = 0;
        int numMoved = 0;
        int totalArrived = 0;
        int *vidToBeSent = (int*) malloc(sizeof(int) * areaWidth * size);
        for (int i = 0; i < areaWidth * size; i++) {
            vidToBeSent[i] = -1;
        }
        int *vidToBeMoved = (int*) malloc(sizeof(int) * GRID_WIDTH * areaWidth);
        for (int i = 0; i < areaWidth * GRID_WIDTH; i++) {
            vidToBeMoved[i] = -1;
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
                hasVehicle = 1;
                Vehicle *v = &vehicles[dequeue(&currentNode->q)];
                grid[i][j].volume = currentNode->q.size;
                if (!(v->current.x == i && v->current.y == j)) {
                    log_fatal("Process %d: Inconsistent location - Current(%d, %d), %d(%d, %d)", rank, i, j, v->current.x, v->current.y);
                    assert(0);
                }
                // Arrived
                if (v->destination.x == i && v->destination.y == j) {
                    numArrived++;
                    log_debug("Vehicle %d arrived at (%d, %d)\n", v->id, i, j);
                    grid[i][j].volume = currentNode->q.size;
                    continue;
                }

                Location nextNode = aStar(v->current.x, v->current.y, v->destination.x, v->destination.y);

                // Pass the vehicle to other process if destination is not within the area.
                v->current.x = nextNode.x;
                v->current.y = nextNode.y;
                grid[i][j].volume = currentNode->q.size;
                if (v->current.y > startAt + areaWidth - 1 || v->current.y < startAt) {
                    int destProcess = nextNode.y/areaWidth;
                    vidToBeSent[destProcess * areaWidth + numVehiclesToBeSent[destProcess]] = v->id;
                    numVehiclesToBeSent[destProcess]++;
                } else {
                    vidToBeMoved[numMoved++] = v->id;
                }
            }
        }
        if (!hasVehicle) {
            log_debug("Process %d doesn't have vehicles", rank);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        // Local movement
        for (int i = 0; i < numMoved; i++) {
            Vehicle *v = &vehicles[vidToBeMoved[i]];
            // log_debug("Vehicle %d moving to (%d, %d)", v->id, v->current.x, v->current.y);
            enqueue(&grid[v->current.x][v->current.y].q, *v);
            grid[v->current.x][v->current.y].volume = grid[v->current.x][v->current.y].q.size;
        }
        // Sync crossing border vehicles
        int *recvLength = (int*)malloc(sizeof(int) * size);
        MPI_Alltoall(numVehiclesToBeSent, 1, MPI_INT, recvLength, 1, MPI_INT, MPI_COMM_WORLD);

        int totalRecvLength = 0;
        for (int i = 0; i < size; i++) {
            totalRecvLength += recvLength[i];
        }

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

                    log_debug("Process %d send v-%d(%d, %d) to Process %d", rank, vehiclesToBeSent[j].id, vehiclesToBeSent[j].current.x, vehiclesToBeSent[j].current.y, i);
                }

                int *sendingBuffer = (int*) malloc(vehicleBufferSize * numVehiclesToBeSent[i] * sizeof(int));
                serializeVehicleArray(vehiclesToBeSent, sendingBuffer, numVehiclesToBeSent[i]);
                MPI_Send(sendingBuffer, vehicleBufferSize * numVehiclesToBeSent[i], MPI_INT, i, 0, MPI_COMM_WORLD);
                free(sendingBuffer);
                free(vehiclesToBeSent);
                sendingBuffer = NULL;
                vehiclesToBeSent = NULL;
            } else {
                int received = 0;
                Vehicle *recvVehicles = (Vehicle *) malloc(totalRecvLength * vehicleBufferSize);
                assert(recvVehicles != NULL);
                for (int j = 0; j < size; j++) {
                    if (j == i || recvLength[j] == 0) {
                        continue;
                    }
                    int *syncBuffer = (int*) malloc(vehicleBufferSize * recvLength[j] * sizeof(int));
                    MPI_Recv(syncBuffer, vehicleBufferSize * recvLength[j], MPI_INT, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    deserializeVehicles(recvVehicles, syncBuffer, recvLength[j]);
                    for (int k = 0; k < recvLength[j]; k++) {
                        if (recvLength[j] == 0) {
                            continue;
                        }
                        log_debug("Process %d received v-%d(%d, %d) from Process %d", rank, recvVehicles[k].id, recvVehicles[k].current.x, recvVehicles[k].current.y, k);
                        tempVehicle[received] = recvVehicles[k];
                        received++;
                    }

                    free(syncBuffer);
                    syncBuffer = NULL;
                }
                free(recvVehicles);
                recvVehicles = NULL;
            }
        }
        free(vidToBeSent);
        free(numVehiclesToBeSent);
        vidToBeSent = NULL;
        numVehiclesToBeSent = NULL;

        for (int i = 0; i < totalRecvLength; i++) {
            // int k = 0;
            // while(!k)
            //     sleep(5);
            Vehicle *v = &tempVehicle[i];
            vehicles[v->id] = *v;
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
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        double end  = MPI_Wtime();
        log_info("Average stepping time: %f", (end-start)/step);
    }
    if (vehicleCount > 0) {
        log_debug("Max number of steps reached.");
    } else {
        log_debug("%d steps used for routing %d vehicles", step, NUM_VEHICLES);
    }
    free(vehicles);
    vehicles = NULL;
    MPI_Finalize();
    return 0;
}
