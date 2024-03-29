#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "heap.h"

const int INFINITY_T = 2147483640;
const int NUM_VERTICES = 1680; // 21 * 80

typedef struct position {
    int rowPos;
    int colPos;
} position_t;

typedef enum {
    Up = 1, Down = 2, Left = 3, Right = 4
} direction_t;

typedef struct character {
    char npc;
    char value[20];
    direction_t direction;
    position_t position;
    long int nextMoveTime;
    char spawn;
} character_t;

typedef struct squares {
    int rowPos;
    int colPos;
    int cost;
    char terrain;
} squares_t;

typedef struct player {
    int rowPos;
    int colPos;
} player_t;

typedef struct terrainMap {
    player_t player;
    int northSouthExit;
    int westEastExit;
    int worldRow;
    int worldCol;
    char terrain[21][80];
    position_t roadPositions[101]; // 80 + 21
} terrainMap_t;

struct terrainMap *world[401][401];

int decorateTerrain(char map[21][80]) {
    int i, j, k;
    char decorations[2] = {'^', '%'};

    for (k = 0; k < (rand() % (20 - 10)) + 10; k++) {
        j = (rand() % (79 - 1)) + 1;
        i = (rand() % (20 - 1)) + 1;

        if (map[i][j] != '#' && map[i][j] != 'M' && map[i][j] != 'C' && map[i][j] != '~') {
            map[i][j] = decorations[rand() % 2];
        }
    }

    return 0;
}

int generateBuildings(terrainMap_t *terrainMap, int row, int col) {
    int pC = (rand() % (70 - 10)) + 10;
    int pM = (rand() % (16 - 3)) + 3;

    int i, j;

    while (terrainMap->terrain[terrainMap->westEastExit - 1][pC - 1] == '#' || terrainMap->terrain[terrainMap->westEastExit - 1][pC] == '#') {
        pC = (rand() % (70 - 10)) + 10;
    }

    while (terrainMap->terrain[pM - 1][terrainMap->northSouthExit - 1] == '#' || terrainMap->terrain[pM][terrainMap->northSouthExit - 1] == '#') {
        pM = (rand() % (16 - 3)) + 3;
    }

    for (i = 1; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            terrainMap->terrain[terrainMap->westEastExit - i][pC - j] = 'C';
        }
    }

    for (i = 0; i < 2; i++) {
        for (j = 1; j < 3; j++) {
            terrainMap->terrain[pM - i][terrainMap->northSouthExit - j] = 'M';
        }
    }

    return 0;  
}

int generatePaths(terrainMap_t *terrainMap, int *currWorldRow, int *currWorldCol) {
    // Will need to update to check for existence of exits
    int i, j, k = 0;
    int rowStart = 0, colStart = 0;
    int rowEnd = 80, colEnd = 21;

    if (*currWorldRow == 0) { // If at the bottom of the world
        colEnd = 20;
    }
    if (*currWorldRow == 401) { // If at the top of the world
        colStart = 1;
    }
    if (*currWorldCol == 0) {
        rowStart = 1;
    }
    if (*currWorldCol == 401) {
        rowEnd = 79;
    }

    for (i = rowStart; i < rowEnd; i++) {
        terrainMap->terrain[terrainMap->westEastExit][i] = '#';
        terrainMap->roadPositions[k].colPos = i;
        terrainMap->roadPositions[k].rowPos = terrainMap->westEastExit;
        k++;
    }

    for (j = colStart; j < colEnd; j++) {
        terrainMap->terrain[j][terrainMap->northSouthExit] = '#';
        terrainMap->roadPositions[k].colPos = terrainMap->northSouthExit;
        terrainMap->roadPositions[k].rowPos = j;
        k++;
    }

    return 0;  
}

int checkSurroundingsForChar(int x, int y, char map[21][80], char checkChar) {
    int i, j;
    
    for (i = -7; i < 8; i++) {
        for (j = -7; j < 8; j++) {
            if (map[y+i][x+j] == checkChar || map[y+i][x+j] == '%' || map[y+i][x+j] == '#') {
                return 1;
            }
        }
    }

    return 0;
}

void generateWater(char map[21][80]) {  
    // generate two of the following in each map. Lake, forest, mountain range
    int x = rand() % 79 + 1;
    int y = rand() % 20 + 1;

    // Ensure that the random point to be used as the center of a monument is at least 5 squares away from any tall grass and
    // the borders of the map
    while(checkSurroundingsForChar(x, y, map, ':') == 1) {
        x = rand() % 79 + 1;
        y = rand() % 20 + 1;
    }

    int i, j;

    int height = (rand() % 7) + 3;
    int width = (rand() % 13) + 5;

    for (i = -height; i < height; i++) {
        for (j = -width; j < width; j++) {
            if (y+i >= 0 && y+i <= 20 && x+j >= 0 && x+j <= 79) {
                if (map[y + i][x + j] != '%' && map[y + i][x + j] != '#') {
                    map[y + i][x + j] = '~';
                }
            }
        }
    }
}

void generateTallGrass(char map[21][80]) {
    int i, j;
    int spotX = (rand() % 29) + 10;
    int spotY = (rand() % 10) + 5;

    while(checkSurroundingsForChar(spotX, spotY, map, '%') == 1) {
        spotX = (rand() % 29) + 10;
        spotY = (rand() % 10) + 5;
    }

    int height = (rand() % (5 - 3)) + 3;
    int width = (rand() % (12 - 7)) + 7;

    for (i = -height; i < height; i++) {
        for (j = -width; j < width; j++) {
            if (spotY+i >= 0 && spotY+i <= 20 && spotX+j >= 0 && spotX+j <= 79) {
                if (map[spotY + i][spotX + j] != '%' && map[spotY + i][spotX + j] != '#') {
                    map[spotY + i][spotX + j] = ':';
                }
            }
        }
    }

    spotX += ((rand() % 20) + 20);
    spotY += ((rand() % 5) - 7);
    
    height = (rand() % 4) + 3;
    width = (rand() % 8) + 4;

    for (i = -height; i < height; i++) {
        for (j = -width; j < width; j++) {
            if (spotY+i >= 0 && spotY+i <= 20 && spotX+j >= 0 && spotX+j <= 79) {
                if (map[spotY + i][spotX + j] != '%' && map[spotY + i][spotX + j] != '#') {
                    map[spotY + i][spotX + j] = ':';
                }
            }
        }
    }
}

void generateExits(terrainMap_t *terrainMap, int *row, int *col) {
    int northSouthExit = (rand() % (69 - 10)) + 10;
    int westEastExit = (rand() % (16 - 3)) + 3;

    terrainMap->northSouthExit = northSouthExit;
    terrainMap->westEastExit = westEastExit;

    if ((*row - 1) >= 0 && (*row + 1) < 401 && (*col - 1) >= 0 && (*col + 1) < 401) {
        if (world[*row - 1][*col] != NULL) {
            terrainMap->northSouthExit = world[*row - 1][*col]->northSouthExit;
        }
        if (world[*row + 1][*col] != NULL) {
            terrainMap->northSouthExit = world[*row + 1][*col]->northSouthExit;    
        }
        if (world[*row][*col - 1] != NULL) {
            terrainMap->westEastExit = world[*row][*col - 1]->westEastExit;
        }
        if (world[*row][*col + 1] != NULL) {
            terrainMap->westEastExit = world[*row][*col + 1]->westEastExit;
        }
    }
}

void placeCharacter(terrainMap_t *terrainMap) {
    // Pick a random road
    int selectedRoad = (rand() % (101 - 0)) + 0;

    // 'Place character' 
    terrainMap->player.rowPos = terrainMap->roadPositions[selectedRoad].rowPos;
    terrainMap->player.colPos = terrainMap->roadPositions[selectedRoad].colPos;
}

void populateHikerCosts(char terrain[21][80], squares_t squares[21][80]) {
    int i, j;

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            if(terrain[i][j] == '^' // Tree
            || terrain[i][j] == '~' // Water
            || (terrain[i][j] == '#' && ((i == 0 || i == 20) || (j == 0 || j == 79)))) { // Gate
                squares[i][j].cost = INFINITY_T;
            }
            if(terrain[i][j] == '%' || terrain[i][j] == '^') { // Boulder
                if(i != 0 && i != 20 && j != 0 && j != 79) {
                    squares[i][j].cost = 15;
                } else {
                    squares[i][j].cost = INFINITY_T;
                }
            }
            if(terrain[i][j] == '#' && i != 0 && i != 20 && j != 0 && j != 79) { // Road, not gate
                squares[i][j].cost = 10;
            }
            if(terrain[i][j] == 'M' || terrain[i][j] == 'C') {
                squares[i][j].cost = 50;
            }
            if(terrain[i][j] == ':') {
                squares[i][j].cost = 15;
            }
            if(terrain[i][j] == '.') {
                squares[i][j].cost = 10;
            }
        }
    }
}

void populateRivalCosts(char terrain[21][80], squares_t squares[21][80]) {
    int i, j;

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            if(terrain[i][j] == '%' // Boulder
            || terrain[i][j] == '^' // Tree
            || terrain[i][j] == '~' // Water
            || (terrain[i][j] == '#' && ((i == 0 || i == 20) || (j == 0 || j == 79)))) { // Gate
                squares[i][j].cost = INFINITY_T;
            }
            if(terrain[i][j] == '#' && i != 0 && i != 20 && j != 0 && j != 79) { // Road, not gate
                squares[i][j].cost = 10;
            }
            if(terrain[i][j] == 'M' || terrain[i][j] == 'C') {
                squares[i][j].cost = 50;
            }
            if(terrain[i][j] == ':') {
                squares[i][j].cost = 20;
            }
            if(terrain[i][j] == '.') {
                squares[i][j].cost = 10;
            }
            
        }
    }
}

void dijkstra(char map[21][80], squares_t squares[21][80], player_t source) {
    static int altCount = 0;
    int i, j;
    int dist[21][80];
    position_t positions[21][80];
    position_t *prev[21][80];
    heap h;
    
    // Using costs of positions as keys, NULL compare function will treat keys as signed integers
    heap_create(&h, 21, NULL);

    // For each 'vertex'
    for(i = 0; i < 21; i++) {
        for(j = 0; j < 80; j++) {
            dist[i][j] = INFINITY_T; // set distance from source to INFINITY_T
            prev[i][j] = malloc(sizeof(position_t));
            positions[i][j].rowPos = i;
            positions[i][j].colPos = j;
            if(i == source.rowPos && j == source.colPos) { // set source dist[] to 0
                dist[i][j] = 0;
            }
            heap_insert(&h, &dist[i][j], &positions[i][j]); // add to queue
        }
    }

    // dist[source.rowPos][source.colPos] = 0;

    heap_entry u;
    // While the queue is not empty
    while(heap_size(&h) > 0) {
        // Remove 'vertex' u from the queue
        heap_delmin(&h, &u.key, &u.value);

        // Get the position of the 'vertex' off of the element stored in the queue
        //  AKA its row and column indices on the map.
        position_t *value = u.value;

        // For each neighbor, v or [i][j], of u
        for (i = value->rowPos - 1; i < value->rowPos + 2; i++) {
            for (j = value->colPos - 1; j < value->colPos + 2; j++) {
                // Make sure neighbors are within bounds of map
                if (i > -1 && i < 21 && j > -1 && j < 80) {
                    // alt = distance to u from source + cost of edge from u to neighbor v
                    int alt = squares[value->rowPos][value->colPos].cost + squares[i][j].cost;
                    if (alt < INFINITY_T && alt < dist[i][j]) { // If alternate path is of lower cost
                        // printf("%d, %d\t", alt, altCount);
                        altCount++;
                        dist[i][j] = alt; // set cost to alt
                        prev[i][j]->rowPos = i;
                        prev[i][j]->colPos = j;
                    }
                }
            }
        }
    }

    // Reassign squares.cost to dist to display the map
    for(i = 0; i < 21; i++) {
        for(j = 0; j < 80; j++) {
            // free(prev[i][j]);
            squares[i][j].cost = dist[i][j];
        }
    }
    
    heap_destroy(&h);
}

position_t findPath(terrainMap_t *terrainMap, int row, int col, character_t *trainer) {
    int i, j;
    // squares arrays hold the dijkstra generated cost map to the player
    squares_t rivalSquares[21][80], hikerSquares[21][80];

    // position_t variable to return
    position_t moveHere;
    // Stand still by default
    moveHere.rowPos = row;
    moveHere.colPos = col;

    if(trainer->npc == 'r') {
        for(i = 0; i < 21; i++) {
            for(j = 0; j < 80; j++) {
                rivalSquares[i][j].rowPos = i;
                rivalSquares[i][j].colPos = j;
                rivalSquares[i][j].terrain = terrainMap->terrain[i][j];
            }
        }

        populateRivalCosts(terrainMap->terrain, rivalSquares);

        dijkstra(terrainMap->terrain, rivalSquares, terrainMap->player);

        for (i = row - 1; i < row + 2; i++) {
            for (j = col - 1; j < col + 2; j++) {
                if (rivalSquares[i][j].cost < rivalSquares[row][col].cost) {
                    moveHere.rowPos = i;
                    moveHere.colPos = j;
                }
            }
        }
    }
    
    if(trainer->npc == 'h') {
        for(i = 0; i < 21; i++) {
            for(j = 0; j < 80; j++) {
                hikerSquares[i][j].rowPos = i;
                hikerSquares[i][j].colPos = j;
                hikerSquares[i][j].terrain = terrainMap->terrain[i][j];
            }
        }

        populateHikerCosts(terrainMap->terrain, hikerSquares);

        dijkstra(terrainMap->terrain, hikerSquares, terrainMap->player);

        for (i = row - 1; i < row + 2; i++) {
            for (j = col - 1; j < col + 2; j++) {
                if (hikerSquares[i][j].cost < hikerSquares[row][col].cost) {
                    moveHere.rowPos = i;
                    moveHere.colPos = j;
                }
            }
        }
    }

    return moveHere;
}

int positionOccupied(int arrSize, position_t arr[arrSize], position_t pos) {
    int i;

    for (i = 0; i < arrSize; i++) {
        if (arr[i].rowPos == pos.rowPos && arr[i].colPos == pos.colPos) {
            return 1;
        }
    }
    return 0;
}

void displayMap(terrainMap_t *terrainMap, int numTrainers, character_t *trainers[numTrainers]) {
    int i, j, k;
    int northMult = 1;
    int westMult = 1;
    char charToPrint;
    char ns = 'N';
    char ew = 'E';

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            charToPrint = terrainMap->terrain[i][j];
            for (k = 0; k < numTrainers; k++) {
                if (i == trainers[k]->position.rowPos && j == trainers[k]->position.colPos && trainers[k]->npc != '@') {
                    // charToPrint = trainers[k]->spawn;
                    charToPrint = trainers[k]->npc;
                }
            }
            if(i == terrainMap->player.rowPos && j == terrainMap->player.colPos) {
                charToPrint = '@';
            }
            printf("%c", charToPrint);
        }
        printf("\n");
    }

    if (terrainMap->worldRow - 200 < 0) {
        northMult = -1;
        ns = 'S';
    }
    if (terrainMap->worldCol - 200 < 0) {
        westMult = -1;
        ew = 'W';
    }

    printf("Coords: %d%c %d%c\n", (terrainMap->worldRow - 200) * northMult, ns, (terrainMap->worldCol - 200) * westMult, ew);
}

void findPosition(character_t *trainer, terrainMap_t *terrainMap, int numTrainers, position_t *positionsUsed[numTrainers]) {
    static int positionsMarked = 0;
    int col = (rand() % (70 - 10)) + 10;
    int row = (rand() % (16 - 3)) + 3;
    position_t pos;
    pos.rowPos = row;
    pos.colPos = col;

    switch(trainer->npc) {
            case '@' :
                break;
            case 'm' : // is a swimmer
                // pick random position that has not been chosen before and is water
                while(terrainMap->terrain[row][col] != '~'
                || positionOccupied(numTrainers, *positionsUsed, pos)) {
                    col = (rand() % (70 - 10)) + 10;
                    row = (rand() % (16 - 3)) + 3;
                }
                // mark trainer position
                trainer->position.rowPos = row;
                trainer->position.colPos = col;
                // printf("Placed %c at [%d, %d]\n", trainer->npc, row, col);
                // add position to positionsUsed
                positionsUsed[positionsMarked]->rowPos = row;
                positionsUsed[positionsMarked]->colPos = col;
                
                positionsMarked++;
                break;
            default : // is any other type of npc
                // pick random position that has not been chosen before and is not a boulder, tree, water, or building
                while(terrainMap->terrain[row][col] == '%'
                || terrainMap->terrain[row][col] == '^'
                || terrainMap->terrain[row][col] == '~'
                || terrainMap->terrain[row][col] == '#'
                || terrainMap->terrain[row][col] == 'M'
                || terrainMap->terrain[row][col] == 'C'
                || positionOccupied(numTrainers, *positionsUsed, pos)) {
                    col = (rand() % (70 - 10)) + 10;
                    row = (rand() % (16 - 3)) + 3;
                }
                // mark trainer position
                trainer->position.rowPos = row;
                trainer->position.colPos = col;
                // printf("Placed %c at [%d, %d]\n", trainer->npc, row, col);
                // for wanderers to know what terrain they spawned in
                trainer->spawn = terrainMap->terrain[row][col];
                // add position to positionsUsed
                positionsUsed[positionsMarked]->rowPos = row;
                positionsUsed[positionsMarked]->colPos = col;
                
                positionsMarked++;
                break;
        }
}

int getMoveCost(terrainMap_t *terrainMap, int row, int col, character_t *trainer) {
    switch(trainer->npc) {
        case '@' :
            switch(terrainMap->terrain[row][col]) {
                case '#' :
                    return 10;
                case 'M' :
                    return 10;
                case 'C' :
                    return 10;
                case ':' :
                    return 20;
                case '.' :
                    return 10;
                default :
                    return INFINITY_T;
            }
            break;
        case 'h' :
            switch(terrainMap->terrain[row][col]) {
                case '#' :
                    return 10;
                case 'M' :
                    return 50;
                case 'C' :
                    return 50;
                case ':' :
                    return 15;
                case '.' :
                    return 10;
                default :
                    return INFINITY_T;
            }
            break;
        case 'm' :
            switch(terrainMap->terrain[row][col]) {
                case '~' :
                    return 7;
                default :
                    return INFINITY_T;
            }
            break;
        case 'w' :
            if (terrainMap->terrain[row][col] == trainer->spawn) {
                return 10;
            } else {
                return INFINITY_T;
            }
            break;
        default :
            switch(terrainMap->terrain[row][col]) {
                case '#' :
                    return 10;
                case 'M' :
                    return 50;
                case 'C' :
                    return 50;
                case ':' :
                    return 20;
                case '.' :
                    return 10;
                case '~' :
                    return INFINITY_T;
                default :
                    return INFINITY_T;
            }
            break;
    }
    return INFINITY_T;
}

int positionNotOccupied(int row, int col, int numTrainers, character_t *trainers[numTrainers]) {
    int i;

    for (i = 0; i < numTrainers; i++) {
        if (trainers[i]->position.rowPos == row && trainers[i]->position.colPos == col) {
            return 0;
        }
    }

    return 1;
}

void generateTrainers(terrainMap_t *terrainMap, int numTrainers) {
    // pick random assortment of trainers, including at least one hiker and one rival
    // unless numTrainers < 2
    int i;
    // make room for player
    numTrainers++;

    character_t *trainers[numTrainers];
    char trainerOptions[7] = {'r', 'h', 'p', 'w', 's', 'e', 'm'};

    for (i = 0; i < numTrainers; i++) {
        trainers[i] = malloc(sizeof(*trainers[i]));
    }

    // Fill up trainers[] with random npcs, guaranteeing the first to be a hiker and the second to be a rival, rest are random
    for (i = 0; i < numTrainers - 1; i++) {
        if (i == 0) {
            trainers[i]->npc = 'h';
            trainers[i]->nextMoveTime = 0;
        } else if (i == 1) {
            trainers[i]->npc = 'r';
            trainers[i]->nextMoveTime = 0;
        } else {
            trainers[i]->npc = trainerOptions[rand() % 7];
            // trainers[i]->npc = 'e'; //testing
            trainers[i]->nextMoveTime = 0;
            // heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]);
        }
    }
    
    // Player for the queue to allow the user a turn to move and to redraw the map
    trainers[numTrainers - 1]->npc = '@';
    trainers[numTrainers - 1]->position.rowPos = terrainMap->player.rowPos;
    trainers[numTrainers - 1]->position.colPos = terrainMap->player.colPos;
    trainers[numTrainers - 1]->nextMoveTime = 0;

    // Place all trainers and give pacers, wanderers, and explorers, a random direction to start with
    direction_t directionOptions[4] = {Up, Down, Left, Right};
    position_t *positionsUsed[numTrainers];
    for (i = 0; i < numTrainers; i++) {
        positionsUsed[i] = malloc(sizeof(*positionsUsed[i]));
        findPosition(trainers[i], terrainMap, numTrainers, positionsUsed);

        // Build value string to use in heap
        snprintf(trainers[i]->value, sizeof(trainers[i]->value), "%c %d", trainers[i]->npc, i);

        if (trainers[i]->npc == 'w' || trainers[i]->npc == 'p' || trainers[i]->npc == 'e' || trainers[i]->npc == 'm') {
            trainers[i]->direction = directionOptions[rand() % 4];
        }
    }

    // Insert trainers into queue
    heap characterHeap;
    heap_create(&characterHeap, 9999, NULL);
    for (i = 0; i < numTrainers; i++) {
        heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->value);
    }

    // While the queue of trainers isn't empty, dequeue the trainer with the cheapest next move, make the move, 
    //  then reinsert with old cost + next move cost
    heap_entry u;
    while(heap_delmin(&characterHeap, &u.key, &u.value)) {
        // move the things
        char value[20];
        char *npc;
        char *index;
        long int moveCost = 0;

        // Deconstruct Value String
        strcpy(value, u.value);
        npc = strtok(value, " ");
        index = strtok(NULL, " ");
        int i = atoi(index);

        switch(*npc) {
            case '@' :
                // handle player movement in 1.05
                // printf("Moved Player\n");
                usleep(250000);
                displayMap(terrainMap, numTrainers, trainers);
                // just adding the moveCost to a random position on the map to very inaccurately 'simulate' the player moving
                moveCost = getMoveCost(terrainMap, (rand() % (16 - 3)) + 3, (rand() % (70 - 10)) + 10, trainers[i]);
                if (moveCost < INFINITY_T) {
                    trainers[i]->nextMoveTime += moveCost;
                }
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            case 'r' :
                position_t rivalMove = findPath(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos, trainers[i]);
                trainers[i]->position.rowPos = rivalMove.rowPos;
                trainers[i]->position.colPos = rivalMove.colPos;
                rivalMove = findPath(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos, trainers[i]);
                moveCost = getMoveCost(terrainMap, rivalMove.rowPos, rivalMove.colPos, trainers[i]);
                if (moveCost < INFINITY_T) {
                    trainers[i]->nextMoveTime += moveCost;
                }
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            case 'h' :
                position_t hikerMove = findPath(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos, trainers[i]);
                trainers[i]->position.rowPos = hikerMove.rowPos;
                trainers[i]->position.colPos = hikerMove.colPos;
                hikerMove = findPath(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos, trainers[i]);
                moveCost = getMoveCost(terrainMap, hikerMove.rowPos, hikerMove.colPos, trainers[i]);
                if (moveCost < INFINITY_T) {
                    trainers[i]->nextMoveTime += moveCost;
                }
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            case 'p' :
                if (trainers[i]->direction == Left) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, numTrainers, trainers)) {
                        trainers[i]->position.colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Right;
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, numTrainers, trainers)) {
                        trainers[i]->position.colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Left;
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, numTrainers, trainers)) {
                        trainers[i]->position.rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Up;
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, numTrainers, trainers)) {
                        trainers[i]->position.rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Down;
                    }
                }
                // printf("Moved Pacer\n");
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Pacer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            case 'w' :
                // move in direction until reach edge of spawn terrain then walk in random new direction
                if (trainers[i]->direction == Left) {
                    if (positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] == trainers[i]->spawn) {
                        trainers[i]->position.colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] == trainers[i]->spawn) {
                        trainers[i]->position.colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (positionNotOccupied(trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] == trainers[i]->spawn) {
                        trainers[i]->position.rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (positionNotOccupied(trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] == trainers[i]->spawn) {
                        trainers[i]->position.rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                // printf("Moved Wanderer\n");
                // usleep(250000);
                // displayMap(terrainMap, numTrainers, trainers);
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Wanderer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            case 's' :
                // Sentries don't move
                break;
            case 'e' :
                // move in direction until reach impassable terrain (boulder, tree, building, or water) then walk in random new direction
                if (trainers[i]->direction == Left) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, numTrainers, trainers)) {
                        trainers[i]->position.colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, numTrainers, trainers)) {
                        trainers[i]->position.colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, numTrainers, trainers)) {
                        trainers[i]->position.rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != '%'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != '^'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != '~'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] != 'C'
                    && positionNotOccupied(trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, numTrainers, trainers)) {
                        trainers[i]->position.rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                // printf("Moved Explorer\n");
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Explorer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            case 'm' :
                // move in direction until reach edge of spawn terrain then walk in random new direction
                // if player is cardinally adjacent/on edge of water directly north, south, west, or east, move towards player
                if (trainers[i]->direction == Left) {
                    if (positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos - 1] == '~') {
                        trainers[i]->position.colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (positionNotOccupied(trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos][trainers[i]->position.colPos + 1] == '~') {
                        trainers[i]->position.colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos, trainers[i]->position.colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (positionNotOccupied(trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos + 1][trainers[i]->position.colPos] == '~') {
                        trainers[i]->position.rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos + 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (positionNotOccupied(trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, numTrainers, trainers)
                    && terrainMap->terrain[trainers[i]->position.rowPos - 1][trainers[i]->position.colPos] == '~') {
                        trainers[i]->position.rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->position.rowPos - 1, trainers[i]->position.colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                // printf("Moved Swimmer\n");
                // usleep(250000);
                // displayMap(terrainMap, numTrainers, trainers);
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Swimmer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            default :
                break;
        }
    }

    heap_destroy(&characterHeap);
}

struct terrainMap * generateTerrain(int *a, int *b, int firstGeneration, int numTrainers) {
    srand(time(NULL));
    
    struct terrainMap *terrainMap = malloc(sizeof(*terrainMap));

    terrainMap->worldRow = *a;
    terrainMap->worldCol = *b;

    int i, j;

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            if (i == 0 || i == 20 || j == 0 || j == 79) {
                terrainMap->terrain[i][j] = '%';
            } else {
                terrainMap->terrain[i][j] = '.';
            }
        }
    }

    double chance = (rand() / (RAND_MAX / 1.00));
    double bldngSpawnChance = abs(*a - 200) + abs(*b - 200);
    bldngSpawnChance *= -45.00;
    bldngSpawnChance /= 400.00;
    bldngSpawnChance += 50.00;
    bldngSpawnChance /= 100.00;

    generateExits(terrainMap, a, b);
    generateTallGrass(terrainMap->terrain);
    generateWater(terrainMap->terrain);
    generatePaths(terrainMap, a, b);
    if ((chance < bldngSpawnChance && chance > 0.00) || firstGeneration) {
        generateBuildings(terrainMap, *a, *b);
    }
    decorateTerrain(terrainMap->terrain);
    placeCharacter(terrainMap);
    generateTrainers(terrainMap, numTrainers);

    return terrainMap;
}

int main(int argc, char *argv[]) {
    // char userInput[13];
    int i, j;
    // int quit = 0;
    // char *move;
    // char *flyRow;
    // char *flyCol;
    int currWorldRow = 200;
    int currWorldCol = 200;
    int numTrainers = 8; // Default number of trainers = 8

    // If the user passed --numtrainers
    if(argv[1]) {
        if (strcmp(argv[1], "--numtrainers") == 0) {
            // Generate terrain with the number they passed
            numTrainers = atoi(argv[2]);
        }
    }
    
    world[currWorldRow][currWorldCol] = generateTerrain(&currWorldRow, &currWorldCol, 1, numTrainers);
    
    // while (!quit) {
    //     printf("~: ");

    //     fgets(userInput, 13, stdin);
    //     move = strtok(userInput, " ");
    //     flyRow = strtok(NULL, " ");
    //     flyCol = strtok(NULL, " ");
        
    //     if (*move == 'n') {
    //         if (currWorldRow < 401) {
    //             currWorldRow++;
    //         }
    //     } else if (*move == 's') {
    //         if (currWorldRow > 0) {
    //             currWorldRow--;
    //         }
    //     } else if (*move == 'w') {
    //         if (currWorldCol > 0) {
    //             currWorldCol--;
    //         }
    //     } else if (*move == 'e') {
    //         if (currWorldCol < 401) {
    //             currWorldCol++;
    //         }
    //     } else if (*move == 'f') {
    //         if (flyRow != NULL 
    //         && flyCol != NULL 
    //         && (atoi(flyRow) + 200) < 401 
    //         && (atoi(flyRow) + 200) >= 0 
    //         && (atoi(flyCol) + 200) < 401
    //         && (atoi(flyCol) + 200) >= 0) {
    //             currWorldRow = atoi(flyRow) + 200;
    //             currWorldCol = atoi(flyCol) + 200;
    //         } else {
    //             printf("\nIncompatible Fly Coordinates\n(Input Coordinates as an ordered pair separated by a space)\n");
    //             continue;
    //         }
    //     } else if (*move == 'q') {
    //         printf("Quitting...\n");
    //         quit = 1;
    //     } else {
    //         printf("Unexpected input... Please try something else.\n");
    //         continue;
    //     }

    //     if (world[currWorldRow][currWorldCol] == NULL) {
    //         world[currWorldRow][currWorldCol] = generateTerrain(&currWorldRow, &currWorldCol, 0, numTrainers);
    //     } else if (!quit){
    //         displayMap(world[currWorldRow][currWorldCol]);
    //     }

    //     printf("\n");
    // }

    for (i = 0; i < 401; i++) {
        for (j = 0; j < 401; j++) {
            free(world[i][j]);
        }
    }

    return 0;
}