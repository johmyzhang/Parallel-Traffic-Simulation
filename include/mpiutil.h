//
// Created by Johnny on 11/23/24.
//

#ifndef MPIUTIL_H
#define MPIUTIL_H
#include "entity.h"
#include "/usr/local/Cellar/open-mpi/5.0.6/include/mpi.h"

void serializeVehicleArray(Vehicle *vehicles, int* buffer, int size);
void deserializeVehicles(Vehicle *vehicles, int* buffer, int size);
void serializeOneVehicle(Vehicle *vehicle, int* buffer);
void deserializeOneVehicle(Vehicle *vehicle, int* buffer);

#endif //MPIUTIL_H
