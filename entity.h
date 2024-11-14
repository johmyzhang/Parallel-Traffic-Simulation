#ifndef ENTITY_H
#define ENTITY_H

#define MAX_ROADS 100
#define MAX_LOCATIONS 100

#define GRID_WIDTH 10
#define GRID_HEIGHT 10

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Maps represented by a 2-D grid.

typedef struct {
    int id;
    int x;
    int y;
} Location;

typedef struct {
    int id;
    int beginX;
    int endX;
    int beginY;
    int endY;
    int length;
    // max # of vehicles on the road at the same time
    int capacity;
    int directed;

} Road;

void initRoad(Road *road, int id, int beginX, int endX, int beginY, int endY, int capacity, int directed) {
    road->id = id;
    road->beginX = beginX;
    road->beginY =beginY;
    road->endX = endX;
    road->endY = endY;
    road->capacity = capacity;
    road->directed = directed;
    road->length = beginX - endX == 0 ? beginY - endY : beginX - endX;
}

typedef struct {
    Road roads[MAX_ROADS];
    Location locations[MAX_LOCATIONS];
    int numRoads;
    int numLocations;
} Map;

void initMap(Map *map) {
    map->numLocations = 0;
    map->numRoads = 0;
}

void addRoad(Map *map, Road *road) {
    if (map->numRoads < MAX_ROADS) {
        memcpy(&map->roads[map->numRoads++], road, sizeof(Road));
    }
}

void addLocation(Map *map, Location *location) {
    if (map->numLocations < MAX_LOCATIONS) {
        memcpy(&map->locations[map->numLocations++], location, sizeof(Location));
    }
}

void printMap(Map *map) {
    char** marks = (char**) malloc(GRID_HEIGHT * sizeof(char*));
    for (int i = 0; i < GRID_HEIGHT; i++) {
        marks[i] = (char*) malloc(GRID_WIDTH * sizeof(char));
    }
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            marks[i][j] = '.';
        }
    }
    int i;
    int j;
    for (i = 0; i < map->numRoads; i++) {
        int x_b = map->roads[i].beginX;
        int x_e = map->roads[i].endX;
        int y_b = map->roads[i].beginY;
        int y_e = map->roads[i].endY;
        if (map->roads[i].beginX == map->roads[i].endX) {
            for (j = y_b; j <= y_e; j++) {
                marks[x_b][j] = '+';
            }
        } else if (map->roads[i].beginY == map->roads[i].endY) {
            for (j = x_b; j <= x_e; j++) {
                marks[j][y_b] = '+';
            }
        }
    }
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            printf("%c ", marks[i][j]);
        }
        printf("\n");
    }
}

typedef struct {
    int id;
    Location current;
    Location destination;
    Location* route;
} Vehicle;

#endif