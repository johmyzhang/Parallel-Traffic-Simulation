#include <stdio.h>
#include <stdlib.h>
#include "entity.h"

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
}