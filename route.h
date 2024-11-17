//
// Created by Johnny on 11/13/24.
//

#include "entity.h"

#ifndef ROUTE_H
#define ROUTE_H



typedef struct {
    int x, y, cost, priority;
} PriorityQueueNode;

typedef struct {
    PriorityQueueNode nodes[GRID_WIDTH * GRID_HEIGHT];
    int size;
} PriorityQueue;


typedef struct {
    int path_length;
    int next_x;
    int next_y;
} PathResult;

PathResult aStar(int start_x, int start_y, int end_x, int end_y, int rows, int cols, int exclude_x, int exclude_y);


#endif //ROUTE_H