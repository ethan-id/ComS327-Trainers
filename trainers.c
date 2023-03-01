#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "heap.h"

const int INFINITY_T = 9999;
const int NUM_VERTICES = 1680; // 21 * 80

typedef struct position {
    int rowPos;
    int colPos;
} position_t;

typedef enum {
    Rival, Hiker, Pacer, Wanderer, Sentry, Explorer, Swimmer, Player
} npcOptions_t;

typedef struct character {
    npcOptions_t npc;
    position_t position;
    position_t spawnPos;
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

void findPath(terrainMap_t *terrainMap, npcOptions_t chosenNPC) {
    int i, j;
    squares_t rivalSquares[21][80], hikerSquares[21][80];

    if(chosenNPC == Rival) {
        for(i = 0; i < 21; i++) {
            for(j = 0; j < 80; j++) {
                rivalSquares[i][j].rowPos = i;
                rivalSquares[i][j].colPos = j;
                rivalSquares[i][j].terrain = terrainMap->terrain[i][j];
            }
        }

        populateRivalCosts(terrainMap->terrain, rivalSquares);

        dijkstra(terrainMap->terrain, rivalSquares, terrainMap->player);

        // printf("Rival Distance Map:\n");
        // for (i = 0; i < 21; i++) { 
        //     for (j = 0; j < 80; j++) {
        //         if (rivalSquares[i][j].cost == INFINITY_T) {
        //             printf("   ");
        //         } else {
        //             printf("%2d ", rivalSquares[i][j].cost % 100);
        //         }
        //     }
        //     printf("\n");
        // }
    }
    
    if(chosenNPC == Hiker) {
        for(i = 0; i < 21; i++) {
            for(j = 0; j < 80; j++) {
                hikerSquares[i][j].rowPos = i;
                hikerSquares[i][j].colPos = j;
                hikerSquares[i][j].terrain = terrainMap->terrain[i][j];
            }
        }

        populateHikerCosts(terrainMap->terrain, hikerSquares);

        dijkstra(terrainMap->terrain, hikerSquares, terrainMap->player);
        
        // printf("Hiker Distance Map:\n");
        // for (i = 0; i < 21; i++) { 
        //     for (j = 0; j < 80; j++) {
        //         if (hikerSquares[i][j].cost == INFINITY_T) {
        //             printf("   ");
        //         } else {
        //             printf("%2d ", hikerSquares[i][j].cost % 100);
        //         }
        //     }
        //     printf("\n");
        // }
    }
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
                if (i == trainers[k]->position.rowPos && j == trainers[k]->position.colPos && trainers[k]->npc != Player) {
                    // charToPrint = trainers[k]->spawn;
                    switch(trainers[k]->npc) {
                        case Player :
                            charToPrint = '@';
                            break;
                        case Rival :
                            charToPrint = 'r';
                            break;
                        case Hiker :
                            charToPrint = 'h';
                            break;
                        case Pacer :
                            charToPrint = 'p';
                            break;
                        case Wanderer :
                            charToPrint = 'w';
                            break;
                        case Sentry :
                            charToPrint = 's';
                            break;
                        case Explorer :
                            charToPrint = 'e';
                            break;
                        case Swimmer :
                            charToPrint = 'm';
                            break;
                    }
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
            case Player :
                break;
            case Swimmer :
                // pick random position that has not been chosen before and is water
                while(terrainMap->terrain[row][col] != '~'
                || positionOccupied(numTrainers, *positionsUsed, pos)) {
                    col = (rand() % (70 - 10)) + 10;
                    row = (rand() % (16 - 3)) + 3;
                }
                // mark trainer position
                trainer->position.rowPos = row;
                trainer->position.colPos = col;
                // add position to positionsUsed
                positionsUsed[positionsMarked]->rowPos = row;
                positionsUsed[positionsMarked]->colPos = col;
                
                positionsMarked++;
                break;
            default :
                // pick random position that has not been chosen before and is not a boulder, tree, water, or building
                while(terrainMap->terrain[row][col] == '%'
                || terrainMap->terrain[row][col] == '^'
                || terrainMap->terrain[row][col] == '~'
                || terrainMap->terrain[row][col] == 'M'
                || terrainMap->terrain[row][col] == 'N'
                || positionOccupied(numTrainers, *positionsUsed, pos)) {
                    col = (rand() % (70 - 10)) + 10;
                    row = (rand() % (16 - 3)) + 3;
                }
                // mark trainer position
                trainer->position.rowPos = row;
                trainer->position.colPos = col;
                // for wanderers to know what terrain they spawned in
                trainer->spawn = terrainMap->terrain[row][col];
                // add position to positionsUsed
                positionsUsed[positionsMarked]->rowPos = row;
                positionsUsed[positionsMarked]->colPos = col;
                
                positionsMarked++;
                break;
        }
}

void generateTrainers(terrainMap_t *terrainMap, int numTrainers) {
    // pick random assortment of trainers, including at least one hiker and one rival
    // unless numTrainers < 2
    int i;
    // make room for player
    numTrainers++;

    character_t *trainers[numTrainers];
    npcOptions_t trainerOptions[7] = {Rival, Hiker, Pacer, Wanderer, Sentry, Explorer, Swimmer};
    heap characterHeap;
    heap_create(&characterHeap, numTrainers * 2, NULL);

    for (i = 0; i < numTrainers; i++) {
        trainers[i] = malloc(sizeof(*trainers[i]));
    }

    for (i = 0; i < numTrainers - 1; i++) {
        if (i == 0) {
            trainers[i]->npc = Hiker;
            trainers[i]->nextMoveTime = time(NULL);
            heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
        } else if (i == 1) {
            trainers[i]->npc = Rival;
            trainers[i]->nextMoveTime = time(NULL);
            heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
        } else {
            trainers[i]->npc = trainerOptions[rand() % 7];
            trainers[i]->nextMoveTime = time(NULL);
            heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
        }
    }
    
    trainers[numTrainers - 1]->npc = Player;
    trainers[numTrainers - 1]->nextMoveTime = time(NULL);
    heap_insert(&characterHeap, &trainers[numTrainers - 1]->nextMoveTime, &trainers[numTrainers - 1]->npc);

    // place all trainers
    position_t *positionsUsed[numTrainers];
    for (i = 0; i < numTrainers; i++) {
        positionsUsed[i] = malloc(sizeof(*positionsUsed[i]));
        findPosition(trainers[i], terrainMap, numTrainers, positionsUsed);
    }

    heap_entry u;
    while(heap_size(&characterHeap) > 0) {
        // move the things
        heap_delmin(&characterHeap, &u.key, &u.value);

        npcOptions_t *currentCharacter = u.value;
        // long int *nextMoveTime = u.key;

        switch(*currentCharacter) {
            case Player :
                // printf("Moved Player, %ld\n", *nextMoveTime);
                break;
            case Rival :
                // printf("Moved Rival, %ld\n", *nextMoveTime);
                break;
            case Hiker :
                // printf("Moved Hiker, %ld\n", *nextMoveTime);
                break;
            case Pacer :
                // printf("Moved Pacer, %ld\n", *nextMoveTime);
                break;
            case Wanderer :
                // printf("Moved Wanderer, %ld\n", *nextMoveTime);
                break;
            case Sentry :
                // Sentries don't move
                // printf("Moved Sentry, %ld\n", *nextMoveTime);
                break;
            case Explorer :
                // printf("Moved Explorer, %ld\n", *nextMoveTime);
                break;
            case Swimmer :
                // printf("Moved Swimmer, %ld\n", *nextMoveTime);
                break;
            default :
                break;
        }

        // *nextMoveTime = time(NULL);
        if (*currentCharacter == Player) {
            // usleep(500000);
            // *nextMoveTime = 10;
        }

        // heap_insert(&characterHeap, &nextMoveTime, &currentCharacter);
        // printf("Inserted\n");
    }
    displayMap(terrainMap, numTrainers, trainers);
    
    // for (i = 0; i <= numTrainers; i++) {
    //     free(trainers[i]);
    // }

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
    // findPath(terrainMap, Rival);
    // findPath(terrainMap, Hiker);

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
    int numTrainers = 5; // Default number of trainers

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