//
// Created by Johnny on 11/23/24.
//

#include </usr/local/Cellar/open-mpi/5.0.6/include/mpi.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "entity.h"
#include "mpiutil.h"

// Serialize the entire array of vehicles into a contiguous memory block
void serializeVehicleArray(Vehicle *vehicles, int* buffer, int size) {
    assert(vehicles != NULL);
    assert(buffer != NULL);
    // Allocate memory for the buffer
    int *ptr = buffer;

    for (int i = 0; i < size; i++) {
        // Serialize id, current, destination
        memcpy(ptr, &vehicles[i].id, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, &vehicles[i].current.x, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, &vehicles[i].current.y, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, &vehicles[i].destination.x, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, &vehicles[i].destination.y, sizeof(int));
        ptr += sizeof(int);
    }
}

void serializeOneVehicle(Vehicle *vehicle, int* buffer) {

    int *ptr = buffer;
    // Serialize id, current, destination
    memcpy(ptr, &vehicle->id, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &vehicle->current.x, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &vehicle->current.y, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &vehicle->destination.x, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, &vehicle->destination.y, sizeof(int));
}

// Deserialize the buffer into an array of vehicles
void deserializeVehicles(Vehicle *vehicles, int *buffer, int size) {
    int *ptr = buffer;
    assert(ptr != NULL);

    // Deserialize each vehicle
    for (int i = 0; i < size; i++) {
        // Deserialize the static fields (id, current, destination)
        memcpy(&vehicles[i].id, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&vehicles[i].current.x, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&vehicles[i].current.y, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&vehicles[i].destination.x, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&vehicles[i].destination.y, ptr, sizeof(int));
        ptr += sizeof(int);
    }
}
void deserializeOneVehicle(Vehicle *vehicle, int* buffer) {

    int *ptr = buffer;
    assert(ptr != NULL);

    // Deserialize the static fields (id, current, destination)
    memcpy(&vehicle->id, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&vehicle->current.x, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&vehicle->current.y, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&vehicle->destination.x, ptr, sizeof(int));
    ptr += sizeof(int);
    memcpy(&vehicle->destination.y, ptr, sizeof(int));

}

