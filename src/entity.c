//
// Created by Johnny on 11/15/24.
//
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/entity.h"
#include "../include/globals.h"

// Maps represented by a 2-D grid.
const int roads[100][100] = {
    // Major blvd
    {0,0,0,99},
    {0,99,0,0},
    {33,33,0,99},
    {66,66,0,99},
    {0,66,40,40},
    {0,66,80,80},
    // Block 0,0
    {0,32,97,97},
    {4,4,80,97},
    {10,10,80,97},
    {23,23,80,97},
    {10,33,91,91},
    {10,33,87,87},
    {10,33,83,83},
    // Block 0,1
    {34,65,97,97},
    {33,43,93,93},
    {56,66,94,94},
    {33,66,89,89},
    {56,66,92,92},
    {56,66,85,85},
    {33,43,85,85},
    {43,43,80,97},
    {47,47,80,97},
    {51,51,80,97},
    {56,56,80,97},
    //Block 1,0
    {0,33,50,50},
    {0,33,60,60},
    {0,33,70,70},
    {4,4,40,80},
    {9,9,40,80},
    {14,14,40,80},
    {19,19,40,80},
    {24,24,40,80},
    {29,29,40,80},
    // Block 1,1
    {33,50,50,50},
    {33,50,60,60},
    {33,50,70,70},
    {50,58,49,49},
    {58,66,47,47},
    {58,66,53,53},
    {50,66,57,57},
    {50,66,63,63},
    {50,66,67,67},
    {56,66,74,74},
    {37,37,40,80},
    {41,41,40,80},
    {45,45,40,80},
    {50,50,40,80},
    {54,54,40,80},
    {58,58,40,57},
    // Block 0,2
    {0,33,35,35},
    {0,33,30,30},
    {0,33,25,25},
    {0,33,20,20},
    {0,33,15,15},
    {0,33,10,10},
    {15,15,15,40},
    {24,24,0,20},
    {19,19,0,10},
    {14,14,0,10},
    {10,10,0,10},
    {26,33,5,5},
    // Block 1,2
    {33,50,35,35},
    {33,50,30,30},
    {33,66,25,25},
    {33,66,20,20},
    {33,66,15,15},
    {33,66,10,10},
    {33,66,6,6},
    {41,41,30,40},
    {50,50,20,40},
    {58,58,20,40},
    {45,43,0,20},
    {53,53,0,20}
};

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

void initMap(Map *map) {
    map->numLocations = 0;
    map->numRoads = 0;
    for (int i = 0; i < sizeof(roads) / sizeof(roads[0]); i++) {
        Road road;
        initRoad(&road, i, roads[i][0], roads[i][1], roads[i][2], roads[i][3], 100, 0);
        addRoad(map, &road);
    }
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
            marks[i][j] = ' ';
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
    FILE* file = fopen("map.txt", "wa");
    for (i = 0; i < GRID_WIDTH; i++) {
        for (j = 0; j < GRID_HEIGHT; j++) {
            fprintf(file, "%c", marks[j][GRID_HEIGHT - i - 1]);
        }
        fprintf(file,"\n");
    }
    fclose(file);
}