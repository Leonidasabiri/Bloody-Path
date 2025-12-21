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
    map->data = (char**)malloc(height * sizeof(char*));
    map->tile_types = (TileType**)malloc(height * sizeof(TileType*));
    if (!map->data || !map->tile_types) {
        cleanup_and_fail(file, map, 0);
        return NULL;
    }

    // Second pass to read data
    rewind(file);
    // printf("ff\n");
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
                continue;
            }
            if (current_char == 'C') {
                map->tile_types[y][x] = TILE_CHECKPOINT;
                continue;
            }
            if (current_char == 'S') {
                map->tile_types[y][x] = TILE_SPIKE;
                continue;
            }
            if (current_char == '.') {
                map->tile_types[y][x] = TILE_EMPTY;
                continue;
            }
            if (current_char == 'W') {
                map->tile_types[y][x] = TILE_WALL_INDESTRUCTIBLE;
                continue;
            }
            if (current_char == 'E') {
                map->tile_types[y][x] = TILE_EXIT;
                continue;
            }
            if (current_char != '#') {
                map->tile_types[y][x] = TILE_EMPTY;
                continue;
            }

            // Check cardinal neighbors
            int north = isSolid(map, x, y - 1);
            int south = isSolid(map, x, y + 1);
            int west = isSolid(map, x - 1, y);
            int east = isSolid(map, x + 1, y);

            if (north && south && west && east) {
                // Check diagonal neighbors for inner corners
                int north_west = isSolid(map, x - 1, y - 1);
                int north_east = isSolid(map, x + 1, y - 1);
                int south_west = isSolid(map, x - 1, y + 1);
                int south_east = isSolid(map, x + 1, y + 1);

                if (!north_west) map->tile_types[y][x] = TILE_WALL_INNER_BOTTOM_RIGHT_CORNER;
                else if (!north_east) map->tile_types[y][x] = TILE_WALL_INNER_BOTTOM_LEFT_CORNER;
                else if (!south_west) map->tile_types[y][x] = TILE_WALL_INNER_TOP_RIGHT_CORNER;
                else if (!south_east) map->tile_types[y][x] = TILE_WALL_INNER_TOP_LEFT_CORNER;
                else map->tile_types[y][x] = TILE_WALL;
            }
            else if (north && south && west) map->tile_types[y][x] = TILE_WALL_RIGHT_EDGE;
            else if (north && south && east) map->tile_types[y][x] = TILE_WALL_LEFT_EDGE;
            else if (north && west && east) map->tile_types[y][x] = TILE_WALL_BOTTOM_EDGE;
            else if (south && west && east) map->tile_types[y][x] = TILE_WALL_TOP_EDGE;
            else if (north && west) map->tile_types[y][x] = TILE_WALL_BOTTOM_RIGHT_CORNER;
            else if (north && east) map->tile_types[y][x] = TILE_WALL_BOTTOM_LEFT_CORNER;
            else if (south && west) map->tile_types[y][x] = TILE_WALL_TOP_RIGHT_CORNER;
            else if (south && east) map->tile_types[y][x] = TILE_WALL_TOP_LEFT_CORNER;
            else {
                // Fallback for single lines, etc.
                if (north || south) map->tile_types[y][x] = TILE_WALL; // Vertical
                else if (west || east) map->tile_types[y][x] = TILE_WALL; // Horizontal
                else map->tile_types[y][x] = TILE_WALL; // Pillar
            }
        }
    }
}

// --- Checkpoint Collection Logic ---

static void collectCheckpoints(Map* map) {
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

static void findExit(Map* map) {
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
