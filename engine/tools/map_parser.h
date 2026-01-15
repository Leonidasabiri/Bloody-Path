#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include "../rendering/renderer.h"

// Represents the different types of tiles after parsing
typedef enum {
    TILE_EMPTY,
    TILE_PLAYER_START,
    TILE_CHECKPOINT,
    TILE_EXIT,
    TILE_SPIKE,
    TILE_SPIKE_BLOODY,
    TILE_SPIKE_INVERSED,
    TILE_SPIKE_INVERSED_BLOODY,
    TILE_WALL_INDESTRUCTIBLE,

    // Auto-tiled wall pieces
    TILE_WALL, // A generic, internal wall piece
    TILE_WALL_TOP_EDGE,
    TILE_WALL_BOTTOM_EDGE,
    TILE_WALL_LEFT_EDGE,
    TILE_WALL_RIGHT_EDGE,
    TILE_WALL_TOP_LEFT_CORNER,
    TILE_WALL_TOP_RIGHT_CORNER,
    TILE_WALL_BOTTOM_LEFT_CORNER,
    TILE_WALL_BOTTOM_RIGHT_CORNER,
    TILE_WALL_INNER_TOP_LEFT_CORNER,
    TILE_WALL_INNER_TOP_RIGHT_CORNER,
    TILE_WALL_INNER_BOTTOM_LEFT_CORNER,
    TILE_WALL_INNER_BOTTOM_RIGHT_CORNER,
} TileType;

// Represents a checkpoint location
typedef struct {
    int x;
    int y;
    bool activated; // Whether the checkpoint has been touched by the player
} Checkpoint;

// Represents the exit/goal location
typedef struct {
    int x;
    int y;
} Exit;

// Represents the game map
typedef struct {
    char** data;
    TileType** tile_types;
    int width;
    int height;
    Checkpoint* checkpoints; // Array of checkpoints in the map
    int checkpointCount; // Number of checkpoints
    Exit exit; // The level exit (only one per map)
    bool hasExit; // Whether the map has an exit tile
    texture_t empty_tile_texture;
    texture_t wall_texture;
    texture_t check_point_texture;
    texture_t exit_texture;
    texture_t spike_texture;
} Map;

/**
 * @brief Loads a map from a text file and processes it for auto-tiling.
 * 
 * @param filename The path to the map file.
 * @return A pointer to the loaded Map, or NULL on failure.
 */
Map* loadMap(const char* filename);

/**
 * @brief Frees the memory allocated for a Map.
 * 
 * @param map The map to destroy.
 */
void destroyMap(Map* map);

/**
 * @brief Checks if the player is colliding with any checkpoint and activates it.
 * Updates player's checkpoint coordinates if a new checkpoint is touched.
 * 
 * @param map The game map containing checkpoints.
 * @param playerX Player's X position in grid coordinates.
 * @param playerY Player's Y position in grid coordinates.
 * @param checkpointX Pointer to store the checkpoint X coordinate.
 * @param checkpointY Pointer to store the checkpoint Y coordinate.
 * @return true if a new checkpoint was activated, false otherwise.
 */
bool checkCheckpointCollision(Map* map, int playerX, int playerY, float* checkpointX, float* checkpointY);

/**
 * @brief Resets all checkpoints in the map to inactive state.
 * 
 * @param map The game map.
 */
void resetCheckpoints(Map* map);

/**
 * @brief Checks if the player is colliding with the exit tile.
 * 
 * @param map The game map containing the exit.
 * @param playerX Player's X position in grid coordinates.
 * @param playerY Player's Y position in grid coordinates.
 * @return true if the player reached the exit, false otherwise.
 */
bool checkExitCollision(Map* map, int playerX, int playerY);
void collectCheckpoints(Map* map);
void findExit(Map* map);

#endif // MAP_PARSER_H