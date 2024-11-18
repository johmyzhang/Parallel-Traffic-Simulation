#ifndef ENTITY_H
#define ENTITY_H

#include "globals.h"


#define GRID_WIDTH 100
#define GRID_HEIGHT 100
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

typedef struct {
    Road roads[MAX_ROADS];
    Location locations[MAX_LOCATIONS];
    int numRoads;
    int numLocations;
} Map;

typedef struct {
    int id;
    Location current;
    Location destination;
    Location* route;
    int activate;
} Vehicle;

typedef struct {
    int x;
    int y;
    int weight;
} Edge;

typedef struct {
    Edge edges[4];
    int edgeCount;
    int volume;
    int capacity;
} Node;

void initRoad(Road *road, int id, int beginX, int endX, int beginY, int endY, int capacity, int directed);

void initMap(Map *map);

void addRoad(Map *map, Road *road);

void addLocation(Map *map, Location *location);

void printMap(Map *map) ;

#endif