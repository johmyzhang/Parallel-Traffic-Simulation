//
// Created by Johnny on 11/15/24.
//
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "entity.h"
#include "route.h"

#include <assert.h>
#include <log.h>
#include <stdlib.h>
#include <time.h>

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

int heuristic(const int x1, const int y1, const int x2, const int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

static void printPath(Route *route, int parent[GRID_WIDTH][GRID_HEIGHT][2], int x, int y) {
    if (parent[x][y][0] == -1 && parent[x][y][1] == -1) {
        route->locations[route->size++] = (Location){x, y};
        return;
    }
    printPath(route, parent, parent[x][y][0], parent[x][y][1]);
    route->locations[route->size++] = (Location){x, y};
}

RoutingResult aStar(const int start_x, const int start_y, const int end_x, const int end_y) {
    int dist[GRID_WIDTH][GRID_HEIGHT];
    int parent[GRID_WIDTH][GRID_HEIGHT][2];
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            dist[i][j] = INF;
            parent[i][j][0] = -1; // Initialize parent to (-1, -1)
            parent[i][j][1] = -1;
        }
    }

    PriorityQueue pq;
    initializePriorityQueue(&pq);

    dist[start_x][start_y] = 0;
    push(&pq, start_x, start_y, 0, heuristic(start_x, start_y, end_x, end_y));
    int pathTaken = 0;
    while (!isEmpty(&pq)) {
        PriorityQueueNode current = pop(&pq);
        int x = current.x, y = current.y;

        if (x == end_x && y == end_y) {
            Route route;
            route.size = 0;
            route.locations = (Location*)malloc((pathTaken + 1)* sizeof(Location));
            printPath(&route, parent, x, y);
            RoutingResult result;
            result.cost = dist[x][y];
            result.route = route;
            result.pathTaken = pathTaken;
            return result;
        }

        for (int i = 0; i < grid[x][y].edgeCount; i++) {
            Edge edge = grid[x][y].edges[i];
            int newCost = dist[x][y] + edge.weight + grid[edge.x][edge.y].q.size;
            if (newCost < dist[edge.x][edge.y]) {
                parent[edge.x][edge.y][0] = x; // Update parent
                parent[edge.x][edge.y][1] = y;
                dist[edge.x][edge.y] = newCost;
                push(&pq, edge.x, edge.y, newCost, newCost + heuristic(edge.x, edge.y, end_x, end_y));
            }
        }
        pathTaken++;
    }
    return (RoutingResult){-1, NULL, -1, -1};
}

// Revised function to add edges between nodes
void addEdge(Node *node, int x, int y, int weight) {
    if (node->edgeCount < 4) {
        node->edges[node->edgeCount] = (Edge){x, y, weight};
        node->edgeCount++;
    }
}

void initializeGrid(const Map *map) {
    // Initialize grid with empty nodes
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].edgeCount = 0;
            grid[i][j].volume = 0;
            initVehicleQueue(&grid[i][j].q);
        }
    }

    // Set up edges in the grid based on roads
    for (int i = 0; i < map->numRoads; i++) {
        Location start = (Location){map->roads[i].beginX, map->roads[i].beginY};
        Location end = (Location){map->roads[i].endX, map->roads[i].endY};

        // If the road is vertical
        if (start.x == end.x) {
            for (int y = start.y; y <= end.y; y++) {
                if (y + 1 <= end.y) {
                    addEdge(&grid[start.x][y], start.x, y + 1, 1); // Downward edge
                }
                if (y - 1 >= start.y) {
                    addEdge(&grid[start.x][y], start.x, y - 1, 1); // Upward edge
                }
            }
        }

        // If the road is horizontal
        if (start.y == end.y) {
            for (int x = start.x; x <= end.x; x++) {
                if (x + 1 <= end.x) {
                    addEdge(&grid[x][start.y], x + 1, start.y, 1); // Rightward edge
                }
                if (x - 1 >= start.x) {
                    addEdge(&grid[x][start.y], x - 1, start.y, 1); // Leftward edge
                }
            }
        }
    }
}

// Function to create vehicles
void initializeVehicles(Vehicle *vehicles) {
    srand(time(NULL));
    for (int i = 0; i < NUM_VEHICLES; i++) {
        vehicles[i].id = i;

        // Ensure vehicles are placed randomly on the grid within valid road areas
        do {
            vehicles[i].current.x = rand() % 30;
            vehicles[i].current.y = rand() % 30;
        } while (grid[vehicles[i].current.x][vehicles[i].current.y].edgeCount == 0);

        // Ensure destination is also randomly chosen but different from the starting point
        do {
            vehicles[i].destination.x = rand() % GRID_WIDTH;
            vehicles[i].destination.y = rand() % GRID_HEIGHT;
        } while ((vehicles[i].destination.x == vehicles[i].current.x && vehicles[i].destination.y == vehicles[i].current.y) ||
                 grid[vehicles[i].destination.x][vehicles[i].destination.y].edgeCount == 0);

        enqueue(&grid[vehicles[i].current.x][vehicles[i].current.y].q, vehicles[i]);
        // Print initial vehicle information
        log_debug("Vehicle %d starts at (%d, %d) and wants to reach (%d, %d)\n",
               vehicles[i].id, vehicles[i].current.x, vehicles[i].current.y,
               vehicles[i].destination.x, vehicles[i].destination.y);
    }
}

void initVehicleQueue(VehicleQueue* queue) {
    assert(queue);
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

bool isVehicleQueueEmpty(VehicleQueue* queue) {
    assert(queue);
    return queue->head == NULL && queue->tail == NULL;
}


void enqueue(VehicleQueue* queue, Vehicle vehicle) {
    assert(queue);
    VehicleNode *newNode = (VehicleNode*) malloc(sizeof(VehicleNode));
    if (newNode == NULL) {
        perror("Memory allocation error\n");
        exit(1);
    }
    newNode->data = vehicle;
    newNode->next = NULL;
    if (isVehicleQueueEmpty(queue)) {
        queue->head = queue->tail = newNode;
    } else {
        queue->tail->next = newNode;
        queue->tail = queue->tail->next;
    }
    queue->size++;
}

int dequeue(VehicleQueue* queue) {
    assert(queue);
    assert(!isVehicleQueueEmpty(queue));
    const int vid = peek(queue).id;
    if (queue->head == queue->tail) {
        free(queue->head);
        queue->head = queue->tail = NULL;
    } else {
        VehicleNode *newNode = queue->head->next;
        free(queue->head);
        queue->head = newNode;
    }

    queue->size--;

    return vid;
}

Vehicle peek(VehicleQueue* queue) {
    assert(queue);
    assert(!isVehicleQueueEmpty(queue));
    return queue->head->data;
}

void deleteQueue(VehicleQueue* queue) {
    assert(queue);
    assert(!isVehicleQueueEmpty(queue));

    VehicleNode *node = queue->head;

    while (node) {
        VehicleNode *next = node->next;
        free(queue->head);
        node = next;
    }
    queue->head = queue->tail = NULL;
    queue->size = 0;
}
