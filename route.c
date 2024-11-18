//
// Created by Johnny on 11/15/24.
//
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "entity.h"
#include "route.h"

#include <stdlib.h>

#define INF INT_MAX

extern Node grid[GRID_WIDTH][GRID_HEIGHT];

static void initializePriorityQueue(PriorityQueue* pq) {
    pq->size = 0;
}

static void push(PriorityQueue* pq, int x, int y, int cost, int priority) {
    pq->nodes[pq->size] = (PriorityQueueNode){x, y, cost, priority};
    int i = pq->size++;
    while (i > 0 && pq->nodes[(i - 1) / 2].priority > pq->nodes[i].priority) {
        PriorityQueueNode temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[(i - 1) / 2];
        pq->nodes[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }
}

static PriorityQueueNode pop(PriorityQueue* pq) {
    const PriorityQueueNode top = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->size];
    int i = 0;
    while (2 * i + 1 < pq->size) {
        const int left = 2 * i + 1;
        const int right = 2 * i + 2;
        int smallest = i;
        if (pq->nodes[left].priority < pq->nodes[smallest].priority) smallest = left;
        if (right < pq->size && pq->nodes[right].priority < pq->nodes[smallest].priority) smallest = right;
        if (smallest == i) break;
        const PriorityQueueNode temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;
        i = smallest;
    }
    return top;
}

static int isEmpty(PriorityQueue* pq) {
    return pq->size == 0;
}

static int heuristic(const int x1, const int y1, const int x2, const int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

static void printPath(int parent[GRID_WIDTH][GRID_HEIGHT][2], int x, int y) {
    if (parent[x][y][0] == -1 && parent[x][y][1] == -1) {
        printf("(%d, %d) ", x, y);
        return;
    }
    printPath(parent, parent[x][y][0], parent[x][y][1]);
    printf("-> (%d, %d) ", x, y);
}

PathResult aStar(int start_x, int start_y, int end_x, int end_y, int rows, int cols, int exclude_x, int exclude_y) {
    int dist[GRID_WIDTH][GRID_HEIGHT];
    int parent[GRID_WIDTH][GRID_HEIGHT][2];
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            dist[i][j] = INF;
            parent[i][j][0] = -1; // Initialize parent to (-1, -1)
            parent[i][j][1] = -1;
        }
    }

    PriorityQueue pq;
    initializePriorityQueue(&pq);

    dist[start_x][start_y] = 0;
    push(&pq, start_x, start_y, 0, heuristic(start_x, start_y, end_x, end_y));

    while (!isEmpty(&pq)) {
        PriorityQueueNode current = pop(&pq);
        int x = current.x, y = current.y;

        if (x == end_x && y == end_y) {
            // printf("Shortest path: ");
            // printPath(parent, x, y);
            // printf("\n");
            int curr_x = x;
            int curr_y = y;
            int path_length = 0;
            int path_x[GRID_WIDTH * GRID_HEIGHT];
            int path_y[GRID_WIDTH * GRID_HEIGHT];

            while (!(curr_x == start_x && curr_y == start_y)) {
                path_x[path_length] = curr_x;
                path_y[path_length] = curr_y;
                path_length++;
                int temp_x = parent[curr_x][curr_y][0];
                int temp_y = parent[curr_x][curr_y][1];
                curr_x = temp_x;
                curr_y = temp_y;
            }

            PathResult result;
            result.path_length = dist[x][y];
            result.next_x = path_x[path_length - 1];
            result.next_y = path_y[path_length - 1];
            return result;
        }
        for (int i = 0; i < grid[x][y].edgeCount; i++) {
            Edge edge = grid[x][y].edges[i];
            int edge_x = edge.x;
            int edge_y = edge.y;

            // Skip if the node is occupied or is the excluded node
            if (grid[edge_x][edge_y].capacity == grid[edge_x][edge_y].volume || (exclude_x >= 0 && edge_x == exclude_x && edge_y == exclude_y)) {
                continue;
            }

            int newCost = dist[x][y] + edge.weight;
            if (newCost < dist[edge_x][edge_y]) {
                parent[edge_x][edge_y][0] = x; // Update parent
                parent[edge_x][edge_y][1] = y;
                dist[edge_x][edge_y] = newCost;
                push(&pq, edge_x, edge_y, newCost, newCost + heuristic(edge_x, edge_y, end_x, end_y));
            }
        }
    }
    PathResult result;
    result.path_length = -1; // No path found
    return result;
}
