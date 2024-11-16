//
// Created by Johnny on 11/13/24.
//

#ifndef ROUTE_H
#define ROUTE_H

typedef struct {
    int x, y, weight;
} Edge;

typedef struct {
    Edge edges[4]; // Maximum 4 neighbors (up, down, left, right)
    int edgeCount;
} Node;

typedef struct {
    int x, y, cost, priority;
} PriorityQueueNode;

typedef struct {
    PriorityQueueNode nodes[GRID_WIDTH * GRID_HEIGHT];
    int size;
} PriorityQueue;

static void initializePriorityQueue(PriorityQueue* pq);

static void push(PriorityQueue* pq, int x, int y, int cost, int priority);

static PriorityQueueNode pop(PriorityQueue* pq);

static int isEmpty(PriorityQueue* pq);

static int heuristic(int x1,  int y1,  int x2,  int y2);

static void printPath(int parent[GRID_WIDTH][GRID_HEIGHT][2], int x, int y);

int aStar( int start_x,  int start_y, int end_x, int end_y, int rows, int cols);

#endif