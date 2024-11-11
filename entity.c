#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GRID_WIDTH 20
#define GRID_HEIGHT 20

#define MAX_ROADS 10
#define MAX_LOCATIONS 10

// Maps represented by a 2-D grid.

typedef struct {
    int id;
    int x;
    int y;
} Location;

typedef struct {
    int id;
    int beginX;
    int beginY;
    int endY;
    // Length = EndY-BeginY
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