#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <time.h>

// Constants for simulation
#define NUM_REGIONS 4
#define NUM_VEHICLES 1000
#define MAX_ROAD_NODES 100
#define MAX_CARS_PER_REGION 50

// Structure to hold vehicle information
typedef struct {
    int id;
    bool route_changed;
    double time_with_route_change;
    double time_without_route_change;
} Vehicle;

// Function to simulate vehicle route recalculation
void recalculate_route(Vehicle *vehicle, int current_region) {
    // Simulated logic for recalculating a route
    int new_route = rand() % MAX_ROAD_NODES; // Randomly choose a new route node
    vehicle->route_changed = (rand() % 2 == 0); // Randomly decide if route changes

    if (vehicle->route_changed) {
        vehicle->time_with_route_change = (rand() % 10) + 10; // Simulate longer travel time
        printf("Vehicle %d in region %d recalculated route to node %d (time with change: %.2f)\n", 
               vehicle->id, current_region, new_route, vehicle->time_with_route_change);
    } else {
        vehicle->time_without_route_change = (rand() % 5) + 5; // Simulate normal travel time
        printf("Vehicle %d in region %d kept original route (time without change: %.2f)\n", 
               vehicle->id, current_region, vehicle->time_without_route_change);
    }
}

// Function to simulate traffic data synchronization between regions
void synchronize_traffic_data(int region_id) {
    // Simulated synchronization logic
    printf("Region %d synchronizing traffic data\n", region_id);

    // Placeholder for broadcasting and receiving updates
    int data_to_broadcast = region_id; // Example traffic data to send
    MPI_Bcast(&data_to_broadcast, 1, MPI_INT, region_id, MPI_COMM_WORLD);
    printf("Region %d broadcasted traffic data\n", region_id);

    // Logic to process received traffic data
    int received_data;
    MPI_Scatter(&data_to_broadcast, 1, MPI_INT, &received_data, 1, MPI_INT, region_id, MPI_COMM_WORLD);
    printf("Region %d received broadcasted traffic data: %d\n", region_id, received_data);
}

// Function to check if a region is full and move vehicles if necessary
void check_and_move_vehicle(int *vehicle_count, int rank, int size) {
    if (*vehicle_count >= MAX_CARS_PER_REGION) {
        int new_region = (rank + 1) % size; // Move to the next region
        printf("Region %d is full. Moving a vehicle to region %d\n", rank, new_region);
        (*vehicle_count)--; // Decrease count in current region

        // Simulate sending the vehicle to the new region
        MPI_Send(vehicle_count, 1, MPI_INT, new_region, 1, MPI_COMM_WORLD);
    }
}

int main(int argc, char** argv) {
    int rank;
    const int size;
    int vehicle_count = 0; // Number of vehicles currently in the region
    int vehicles_with_route_change = 0;
    double total_time_with_change = 0.0;
    double total_time_without_change = 0.0;

    // Initialize MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != NUM_REGIONS) {
        if (rank == 0) {
            fprintf(stderr, "Error: Number of processes must be %d\n", NUM_REGIONS);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Seed for random number generation
    srand(time(NULL) + rank);

    // Initial vehicle distribution (for simplicity, evenly distributed)
    vehicle_count = NUM_VEHICLES / size;
    Vehicle vehicles[NUM_VEHICLES / size];

    // Initialize vehicle data
    for (int i = 0; i < vehicle_count; i++) {
        vehicles[i].id = i + rank * (NUM_VEHICLES / size);
        vehicles[i].route_changed = false;
        vehicles[i].time_with_route_change = 0.0;
        vehicles[i].time_without_route_change = 0.0;
    }

    // Simulation loop for each region
    for (int step = 0; step < 10; step++) { // Example: run 10 simulation steps
        // Check and move vehicles if the region is full
        check_and_move_vehicle(&vehicle_count, rank, size);

        // Simulate traffic data processing and route recalculation
        for (int i = 0; i < vehicle_count; i++) {
            recalculate_route(&vehicles[i], rank);
            if (vehicles[i].route_changed) {
                vehicles_with_route_change++;
                total_time_with_change += vehicles[i].time_with_route_change;
            } else {
                total_time_without_change += vehicles[i].time_without_route_change;
            }
        }

        // Synchronize data across regions using non-blocking communication
        MPI_Request request;
        int traffic_data = rank; // Placeholder traffic data
        for (int dest = 0; dest < size; dest++) {
            if (dest != rank) {
                MPI_Isend(&traffic_data, 1, MPI_INT, dest, 0, MPI_COMM_WORLD, &request);
            }
        }

        int received_data;
        for (int src = 0; src < size; src++) {
            if (src != rank) {
                MPI_Irecv(&received_data, 1, MPI_INT, src, 0, MPI_COMM_WORLD, &request);
                MPI_Wait(&request, MPI_STATUS_IGNORE);
                printf("Region %d received traffic data from region %d\n", rank, src);
            }
        }

        synchronize_traffic_data(rank);
    }

    // Print final statistics
    printf("Region %d: %d vehicles changed routes. Total time with change: %.2f, Total time without change: %.2f\n", 
           rank, vehicles_with_route_change, total_time_with_change, total_time_without_change);

    // Finalize MPI environment
    MPI_Finalize();
    return 0;
}
