//
// Created by Johnny on 11/13/24.
//

#ifndef ROUTE_H
#define ROUTE_H
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "entity.h"

#define MAX_NODES GRID_WIDTH * GRID_HEIGHT
#define INFINITY INT_MAX
typedef struct {
    int weight;
    int volume;
    int capacity;
    struct Edge* next;
} Edge;

typedef struct {
    int id;
    Edge* edges;
} Node;

typedef struct {
    int adj[MAX_NODES][MAX_NODES];
    int numNodes;
} Graph;

// Priority Queue for Dijkstra’s Algorithm
typedef struct {
    Node nodes[MAX_NODES];
    int size;
} PriorityQueue;

// Initialize the graph
void initGraph(Graph *graph, int numNodes) {
    graph->numNodes = numNodes;
    for (int i = 0; i < numNodes; i++) {
        for (int j = 0; j < numNodes; j++) {
            graph->adj[i][j] = (i == j) ? 0 : INFINITY;
        }
    }
}

// Add a bidirectional edge between nodes
void addEdge(Graph *graph, int u, int v, int weight) {
    graph->adj[u][v] = weight;
    graph->adj[v][u] = weight;
}

// Initialize priority queue
void initQueue(PriorityQueue *pq) {
    pq->size = 0;
}

void push(PriorityQueue *pq, int id, int distance) {
    pq->nodes[pq->size].id = id;
    pq->nodes[pq->size].distance = distance;
    pq->size++;
}

Node pop(PriorityQueue *pq) {
    int minIndex = 0;
    for (int i = 1; i < pq->size; i++) {
        if (pq->nodes[i].distance < pq->nodes[minIndex].distance) {
            minIndex = i;
        }
    }
    Node minNode = pq->nodes[minIndex];
    pq->nodes[minIndex] = pq->nodes[--pq->size];
    return minNode;
}

// Dijkstra’s algorithm
void dijkstra(Graph *graph, int start, int end) {
    int dist[MAX_NODES], predecessor[MAX_NODES];
    PriorityQueue pq;
    initQueue(&pq);

    for (int i = 0; i < graph->numNodes; i++) {
        dist[i] = INFINITY;
        predecessor[i] = -1;
    }
    dist[start] = 0;
    push(&pq, start, 0);

    while (pq.size > 0) {
        Node node = pop(&pq);
        int u = node.id;

        if (u == end) break;

        for (int v = 0; v < graph->numNodes; v++) {
            if (graph->adj[u][v] != INFINITY) {
                int alt = dist[u] + graph->adj[u][v];
                if (alt < dist[v]) {
                    dist[v] = alt;
                    predecessor[v] = u;
                    push(&pq, v, alt);
                }
            }
        }
    }

    // Print shortest path
    if (dist[end] == INFINITY) {
        printf("No path from %d to %d\n", start, end);
        return;
    }
    printf("Shortest path from %d to %d (Distance: %d): ", start, end, dist[end]);
    int path[MAX_NODES], path_size = 0;
    for (int at = end; at != -1; at = predecessor[at]) {
        path[path_size++] = at;
    }
    for (int i = path_size - 1; i >= 0; i--) {
        printf("%d ", path[i]);
    }
    printf("\n");
}

int main() {
    Graph graph;
    initGraph(&graph, 5); // Create a graph with 5 nodes

    // Add roads between locations
    addEdge(&graph, 0, 1, 10);
    addEdge(&graph, 0, 2, 3);
    addEdge(&graph, 1, 2, 1);
    addEdge(&graph, 1, 3, 2);
    addEdge(&graph, 2, 3, 8);
    addEdge(&graph, 2, 4, 2);
    addEdge(&graph, 3, 4, 7);

    int start = 0, end = 4;
    dijkstra(&graph, start, end);  // Find shortest path from node 0 to node 4

    return 0;
}

#endif //ROUTE_H
