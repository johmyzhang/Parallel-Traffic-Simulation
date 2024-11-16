#include <stdio.h>
#include <stdlib.h>

#include "entity.h"
#include "route.h"

Node grid[GRID_WIDTH][GRID_HEIGHT];
int main() {
    const int roads[4][4] = {{4,4,0,9}, {0,9,4,4}, {2,2,2,8}, {2,8,2,2}};
    Map map;
    initMap(&map);
    for (int i = 0; i < 4; i++) {
        Road road;
        initRoad(&road, i, roads[i][0], roads[i][1], roads[i][2], roads[i][3], 100, 0);
        addRoad(&map, &road);
    }
    printMap(&map);
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].edgeCount = 0;
        }
    }
    for (int i = 0; i < map.numRoads; i++) {
        int x = map.roads[i].beginX;
        int y = map.roads[i].beginY;
        if (map.roads[i].endX == x) {
            for (int j = 0; j < abs(y - map.roads[i].endY) + 1; j++) {
                if (y + j + 1 <= map.roads[i].endY) {
                    grid[x][y + j].edges[grid[x][y + j].edgeCount] = (Edge){x, y + j + 1, 1};
                    grid[x][y + j].edgeCount++;
                }
                if (y + j - 1 >= map.roads[i].beginY) {
                    grid[x][y + j].edges[grid[x][y + j].edgeCount] = (Edge){x, y + j - 1, 1};
                    grid[x][y + j].edgeCount++;
                }
            }
        } else {
            for (int j = 0; j < abs(x - map.roads[i].endX) + 1; j++) {
                if (x + j + 1 <= map.roads[i].endX) {
                    grid[x + j][y].edges[grid[x + j][y].edgeCount] = (Edge){x + j + 1, y, 1};
                    grid[x + j][y].edgeCount++;
                }
                if (x + j - 1 >= map.roads[i].beginX) {
                    grid[x + j][y].edges[grid[x + j][y].edgeCount] = (Edge){x + j - 1, y, 1};
                    grid[x + j][y].edgeCount++;
                }
            }
        }
    }
    int sp = aStar(4, 0, 2, 8, GRID_WIDTH, GRID_HEIGHT);
    printf("Shortest Path Length: %d\n", sp);
}