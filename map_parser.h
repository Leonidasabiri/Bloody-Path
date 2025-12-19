#ifndef MAP_PARSER_H
#define MAP_PARSER_H

// Represents the different types of tiles after parsing
typedef enum {
    TILE_EMPTY,
    TILE_PLAYER_START,
    TILE_CHECKPOINT,
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

// Represents the game map
typedef struct {
    char** data;
    TileType** tile_types;
    int width;
    int height;
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

#endif // MAP_PARSER_H
