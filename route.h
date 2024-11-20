//
// Created by Johnny on 11/13/24.
//

#ifndef ROUTE_H
#define ROUTE_H
#include <stdbool.h>

typedef struct {
    int x, y, weight;
} Edge;

typedef struct VehicleNode{
    Vehicle data;
    struct VehicleNode* next;
} VehicleNode;

typedef struct {
    VehicleNode* head;
    VehicleNode* tail;
    int size;
} VehicleQueue;

typedef struct {
    Edge edges[4]; // Maximum 4 neighbors (up, down, left, right)
    int edgeCount;
    int volume;
    VehicleQueue q;
} Node;

typedef struct {
    int x, y, cost, priority;
} PriorityQueueNode;

typedef struct {
    PriorityQueueNode nodes[GRID_WIDTH * GRID_HEIGHT];
    int size;
} PriorityQueue;

typedef struct {
    int size;
    Location* locations;
} Route;

typedef struct {
    Route route;
    int pathTaken;
    int cost;
} RoutingResult;

static void initializePriorityQueue(PriorityQueue* pq);

static void push(PriorityQueue* pq, int x, int y, int cost, int priority);

static PriorityQueueNode pop(PriorityQueue* pq);

static int isEmpty(PriorityQueue* pq);

int heuristic(int x1,  int y1,  int x2,  int y2);

static void printPath(Route *route, int parent[GRID_WIDTH][GRID_HEIGHT][2], int x, int y);

RoutingResult aStar(int start_x,  int start_y, int end_x, int end_y);

void initializeGrid(const Map *map);

void addEdge(Node *node, int x, int y, int weight);

void initializeVehicles(Vehicle *vehicles);

void initVehicleQueue(VehicleQueue* queue);

bool isVehicleQueueEmpty(VehicleQueue* queue);

void enqueue(VehicleQueue* queue, Vehicle vehicle);

int dequeue(VehicleQueue* queue);

Vehicle peek(VehicleQueue* queue);

void deleteQueue(VehicleQueue* queue);

#endif