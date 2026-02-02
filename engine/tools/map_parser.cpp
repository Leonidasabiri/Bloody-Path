#include "map_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 512

static void analyzeMap(Map* map);

// Helper to clean up resources on error
static void cleanup_and_fail(FILE* file, Map* map, int allocated_rows) {
    if (file) fclose(file);
    if (map) {
        if (map->data) {
            for (int i = 0; i < allocated_rows; ++i) {
                free(map->data[i]);
            }
            free(map->data);
        }
        if (map->tile_types) {
            for (int i = 0; i < allocated_rows; ++i) {
                free(map->tile_types[i]);
            }
            free(map->tile_types);
        }
        free(map);
    }
}

Map* loadMap(const char* filename) {
        FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening map file");
        return NULL;
    }

    Map* map = (Map*)malloc(sizeof(Map));
    if (!map) {
        fclose(file);
        return NULL;
    }
    map->data = NULL;
    map->tile_types = NULL;
    map->width = 0;
    map->height = 0;

    char line[MAX_LINE_LENGTH];
    int height = 0;
    int width = -1;

    // First pass to determine dimensions
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; // Remove newline chars
        if (strlen(line) > 0) {
            if (width == -1) {
                width = strlen(line);
            } else if ((int)strlen(line) != width) {
                fprintf(stderr, "Map format error: Inconsistent line length.\n");
                cleanup_and_fail(file, map, 0);
                return NULL;
            }
            height++;
        }
    }

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Map is empty or has invalid dimensions.\n");
        cleanup_and_fail(file, map, 0);
        return NULL;
    }

    map->width = width;
    map->height = height;
    map->checkpoints = NULL;
    map->checkpointCount = 0;
    map->hasExit = false;
    map->exit.x = 0;
    map->exit.y = 0;
    map->data = (char**)malloc(height * sizeof(char*));
    map->tile_types = (TileType**)malloc(height * sizeof(TileType*));
    if (!map->data || !map->tile_types) {
        cleanup_and_fail(file, map, 0);
        return NULL;
    }

    // Second pass to read data
    rewind(file);
    int current_row = 0;
    while (fgets(line, sizeof(line), file) && current_row < height) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        map->data[current_row] = (char*)malloc(width + 1);
        map->tile_types[current_row] = (TileType*)malloc(width * sizeof(TileType));
        if (!map->data[current_row] || !map->tile_types[current_row]) {
            cleanup_and_fail(file, map, current_row);
            return NULL;
        }
        strcpy(map->data[current_row], line);
        current_row++;
    }

    fclose(file);

    // Third pass to analyze the map and determine tile types
    analyzeMap(map);

    // Fourth pass to collect checkpoint locations
    collectCheckpoints(map);

    // Fifth pass to find the exit
    findExit(map);

    return map;
}

void destroyMap(Map* map) {
    if (!map) return;
    if (map->data) {
        for (int i = 0; i < map->height; ++i) {
            free(map->data[i]);
        }
        free(map->data);
    }
    if (map->tile_types) {
        for (int i = 0; i < map->height; i++) {
            free(map->tile_types[i]);
        }
        free(map->tile_types);
    }
    free(map);
}

// --- Auto-tiling Logic ---

static int isSolid(Map* map, int x, int y) {
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) {
        return 1; // Treat out-of-bounds as solid for consistent edges
    }
    char tile = map->data[y][x];
    return tile == '#' || tile == 'W';
}

static void analyzeMap(Map* map) {
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            char current_char = map->data[y][x];

            if (current_char == 'P') {
                map->tile_types[y][x] = TILE_PLAYER_START;
            }
            if (current_char == 'C') {
                map->tile_types[y][x] = TILE_CHECKPOINT;
            }
            if (current_char == 'S') {
                map->tile_types[y][x] = TILE_SPIKE;
            }
            if (current_char == '.') {
                map->tile_types[y][x] = TILE_EMPTY;
            }
            if (current_char == 'W') {
                map->tile_types[y][x] = TILE_WALL_INDESTRUCTIBLE;
            }
            if (current_char == 'E') {
                map->tile_types[y][x] = TILE_EXIT;
            }

            // Check cardinal neighbors
            int north = isSolid(map, x, y - 1);
            int south = isSolid(map, x, y + 1);
            int west = isSolid(map, x - 1, y);
            int east = isSolid(map, x + 1, y);

            if (map->tile_types[y][x] == TILE_WALL_INDESTRUCTIBLE)
            {
                if (x <= 0 || (west && !east))
                    map->tile_types[y][x] = TILE_WALL_LEFT_EDGE;
                if (x <= 0 && y <= 0)
                    map->tile_types[y][x] = TILE_WALL_TOP_LEFT_CORNER;
                if (x <= 0 && y >= map->height)
                    map->tile_types[y][x] = TILE_WALL_BOTTOM_LEFT_CORNER;
            }
        }
    }
}

// --- Checkpoint Collection Logic ---
void collectCheckpoints(Map* map) {
    // First pass: count checkpoints
    int count = 0;
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->tile_types[y][x] == TILE_CHECKPOINT) {
                count++;
            }
        }
    }

    if (count == 0) {
        map->checkpoints = NULL;
        map->checkpointCount = 0;
        return;
    }

    // Allocate checkpoint array
    map->checkpoints = (Checkpoint*)malloc(count * sizeof(Checkpoint));
    if (!map->checkpoints) {
        fprintf(stderr, "Failed to allocate memory for checkpoints\n");
        map->checkpointCount = 0;
        return;
    }

    // Second pass: collect checkpoint positions
    int index = 0;
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->tile_types[y][x] == TILE_CHECKPOINT) {
                map->checkpoints[index].x = x;
                map->checkpoints[index].y = y;
                map->checkpoints[index].activated = false;
                index++;
            }
        }
    }

    map->checkpointCount = count;
    printf("Loaded %d checkpoint(s) from map\n", count);
}

// --- Checkpoint Management Functions ---
bool checkCheckpointCollision(Map* map, int playerX, int playerY, float* checkpointX, float* checkpointY) {
    if (!map || !map->checkpoints || map->checkpointCount == 0) {
        return false;
    }


    for (int i = 0; i < map->checkpointCount; i++) {
        Checkpoint* cp = &map->checkpoints[i];
        
        // Check if player is on this checkpoint tile
        if (cp->x == playerX && cp->y == playerY) {
            if (!cp->activated) {
                // New checkpoint activated!
                cp->activated = true;
                *checkpointX = (float)cp->x;
                *checkpointY = (float)cp->y;
                printf("Checkpoint activated at (%d, %d)\n", cp->x, cp->y);
                return true;
            }
        }
    }
    
    return false;
}

void resetCheckpoints(Map* map) {
    if (!map || !map->checkpoints) {
        return;
    }
    
    for (int i = 0; i < map->checkpointCount; i++) {
        map->checkpoints[i].activated = false;
    }
    
    printf("All checkpoints reset\n");
}

// --- Exit Management Functions ---

void findExit(Map* map) {
    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->tile_types[y][x] == TILE_EXIT) {
                map->exit.x = x;
                map->exit.y = y;
                map->hasExit = true;
                printf("Exit found at (%d, %d)\n", x, y);
                return;
            }
        }
    }
    
    printf("Warning: No exit found in map\n");
}

bool checkExitCollision(Map* map, int playerX, int playerY) {
    if (!map || !map->hasExit) {
        return false;
    }
    
    if (map->exit.x == playerX && map->exit.y == playerY) {
        printf("Player reached the exit!\n");
        return true;
    }
    
    return false;
}

void findPlayerStart(Map* map, Player* player) 
{
    if (!map || !player) return;
    float tileWidth = WALL_SPRITE_X;
    float tileHeight = WALL_SPRITE_X;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->data[y][x] == 'P') {
                player->x = x * (WALL_SPRITE_X/((float)SCREEN_WIDTH/2));
                player->y = (map->height - y) * WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);
                player->width = (WALL_SPRITE_X-10)/((float)SCREEN_WIDTH/2);
                player->height = (WALL_SPRITE_X)/((float)SCREEN_HEIGHT/2);
                player->vy = 0;
                player->onGround = false;
                player->checkpointX = (float)x;
                player->checkpointY = (float)y;
                return;
            }
        }
    }
    player->x = SCREEN_WIDTH/2;
    player->y = SCREEN_HEIGHT/2;
    player->width = WALL_SPRITE_X/2;
    player->height = WALL_SPRITE_X;
    player->vy = 0;
    player->onGround = false;
    player->checkpointX = player->x / WALL_SPRITE_X;
    player->checkpointY = player->y / WALL_SPRITE_X;
}

bool checkWallCollision(float x, float y, Map* map) 
{
    if (!map) return true; // Treat no map as a solid wall

    float tileWidth = (float)WALL_SPRITE_X/((float)SCREEN_WIDTH/2);
    float tileHeight = (float)WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);

    int mapX = (int)(x/tileWidth);
    int mapY = map->height - (int)(y/tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return true; // Collide with boundaries
    }

    char tile = map->data[mapY][mapX];
    return tile == 'W' || tile == '#';
}

bool checkSpikeCollision(Player* player, Map* map) 
{
    if (!map) return false;

    float tileWidth = (float)TEXTURE_DEMENSIONS / map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS / map->height;

    // Get player center
    float playerCenterX = player->x + player->width / 2;
    float playerCenterY = player->y + player->height / 2;

    int mapX = (int)(playerCenterX / tileWidth);
    int mapY = (int)(playerCenterY / tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return false;
    }

    if (map->tile_types[mapY][mapX] == TILE_SPIKE || map->tile_types[mapY][mapX] == TILE_SPIKE_BLOODY)
    {
        map->tile_types[mapY][mapX] = TILE_SPIKE_BLOODY;
    }
    return map->tile_types[mapY][mapX] == TILE_SPIKE_BLOODY;
}

bool checkExitCollisionLocal(Player* player, Map* map)
{
    if (!map) return false;

    float tileWidth = (float)TEXTURE_DEMENSIONS / map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS / map->height;

    // Check if ANY part of the player overlaps with the exit tile
    // Calculate which tiles the player occupies
    // Add a small tolerance (2 pixels) to account for wall collision blocking
    float tolerance = 2.0f;
    int leftTile = (int)(player->x / tileWidth);
    int rightTile = (int)((player->x + player->width + tolerance) / tileWidth);
    int topTile = (int)(player->y / tileHeight);
    int bottomTile = (int)((player->y + player->height - 1) / tileHeight);
    
    // Check all tiles the player overlaps with (or is very close to)
    for (int y = topTile; y <= bottomTile; y++) {
        for (int x = leftTile; x <= rightTile; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                if (map->tile_types[y][x] == TILE_EXIT) {
                    printf("Exit collision detected! Player overlapping exit at grid (%d, %d)\n", x, y);
                    return true;
                }
            }
        }
    }

    return false;
}

