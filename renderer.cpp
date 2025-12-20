#include "renderer.h"
#include "config.h"
#include <SDL_image.h>
#include <stdlib.h>
#include <time.h>

// Tile mapping from your tileset (indices based on your selection)
#define TILE_LEFT_WALL 20              // tile 20 (row 2, col 0)
#define TILE_RIGHT_WALL 5              // tile 5 (row 0, col 5)
#define TILE_TOP_LEFT_CORNER 50        // tile 50 (row 5, col 0)
#define TILE_TOP_RIGHT_CORNER 53       // tile 53 (row 5, col 3)
#define TILE_BOTTOM_LEFT_CORNER 45     // tile 45 (row 4, col 5)
#define TILE_BOTTOM_RIGHT_CORNER 40    // tile 40 (row 4, col 0)
#define TILE_TOP_WALL_1 51             // tile 51 (row 5, col 1)
#define TILE_TOP_WALL_2 52             // tile 52 (row 5, col 2)
#define TILE_BOTTOM_WALL_1 44          // tile 44 (row 4, col 4)
#define TILE_BOTTOM_WALL_2 43          // tile 43 (row 4, col 3)
#define TILE_PLATFORM_1 1              // tile 1 (row 0, col 1)
#define TILE_PLATFORM_2 2              // tile 2 (row 0, col 2)
#define TILE_PLATFORM_3 3              // tile 3 (row 0, col 3)
#define TILE_PLATFORM_4 4              // tile 4 (row 0, col 4)
#define TILE_BACKGROUND_1 21           // tile 21 (row 2, col 1)
#define TILE_BACKGROUND_2 22           // tile 22 (row 2, col 2)
#define TILE_BACKGROUND_3 23           // tile 23 (row 2, col 3)
#define TILE_BACKGROUND_4 24           // tile 24 (row 2, col 4)

// Animation frame counts for each animation type
static const int animFrameCounts[] = {
    2,  // ANIM_IDLE - 2 frames (not 4!)
    4,  // ANIM_IDLE_BLINK - 4 frames
    6,  // ANIM_WALK - 6 frames
    8,  // ANIM_RUN - 8 frames
    4,  // ANIM_DUCK - 4 frames
    4,  // ANIM_JUMP - 4 frames
    6,  // ANIM_DISAPPEAR - 6 frames
    8,  // ANIM_DIE - 8 frames
    6   // ANIM_ATTACK - 6 frames
};

static SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return NULL;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
    }
    
    return texture;
}

static int getRandomVariant(int baseIndex, int variantCount) {
    return baseIndex + (rand() % variantCount);
}

void initializeTileVariants(GameRenderer* gameRenderer, Map* map) {
    // Free existing variants if any
    if (gameRenderer->tileVariants) {
        for (int y = 0; y < gameRenderer->mapHeight; y++) {
            free(gameRenderer->tileVariants[y]);
        }
        free(gameRenderer->tileVariants);
    }

    // Allocate new variant array
    gameRenderer->mapWidth = map->width;
    gameRenderer->mapHeight = map->height;
    gameRenderer->tileVariants = (int**)malloc(sizeof(int*) * map->height);
    
    printf("=== Initializing Tile Variants ===\n");
    int bottomWallCount = 0;
    int bottomCornerCount = 0;
    
    for (int y = 0; y < map->height; y++) {
        gameRenderer->tileVariants[y] = (int*)malloc(sizeof(int) * map->width);
        for (int x = 0; x < map->width; x++) {
            TileType type = map->tile_types[y][x];
            
            // Assign tiles based on tile type detected by map parser
            switch (type) {
                case TILE_EMPTY:
                case TILE_PLAYER_START:
                case TILE_CHECKPOINT:
                case TILE_EXIT:
                case TILE_SPIKE:
                    // Background tile - randomize between 4 variants
                    gameRenderer->tileVariants[y][x] = TILE_BACKGROUND_1 + (rand() % 4);
                    break;
                    
                case TILE_WALL_TOP_EDGE:
                    // Top wall - use bottom wall tiles flipped upside down
                    gameRenderer->tileVariants[y][x] = (rand() % 2 == 0) ? TILE_BOTTOM_WALL_1 : TILE_BOTTOM_WALL_2;
                    break;
                    
                case TILE_WALL_BOTTOM_EDGE:
                    // Bottom wall - randomize between 2 variants
                    gameRenderer->tileVariants[y][x] = (rand() % 2 == 0) ? TILE_BOTTOM_WALL_1 : TILE_BOTTOM_WALL_2;
                    bottomWallCount++;
                    printf("Bottom wall at (%d,%d) using tile %d\n", x, y, gameRenderer->tileVariants[y][x]);
                    break;
                    
                case TILE_WALL_LEFT_EDGE:
                    gameRenderer->tileVariants[y][x] = TILE_LEFT_WALL;
                    break;
                    
                case TILE_WALL_RIGHT_EDGE:
                    gameRenderer->tileVariants[y][x] = TILE_RIGHT_WALL;
                    break;
                    
                case TILE_WALL_TOP_LEFT_CORNER:
                    gameRenderer->tileVariants[y][x] = TILE_TOP_LEFT_CORNER;
                    break;
                    
                case TILE_WALL_TOP_RIGHT_CORNER:
                    gameRenderer->tileVariants[y][x] = TILE_TOP_RIGHT_CORNER;
                    break;
                    
                case TILE_WALL_BOTTOM_LEFT_CORNER:
                    gameRenderer->tileVariants[y][x] = TILE_BOTTOM_LEFT_CORNER;
                    bottomCornerCount++;
                    printf("Bottom-left corner at (%d,%d) using tile %d\n", x, y, TILE_BOTTOM_LEFT_CORNER);
                    break;
                    
                case TILE_WALL_BOTTOM_RIGHT_CORNER:
                    gameRenderer->tileVariants[y][x] = TILE_BOTTOM_RIGHT_CORNER;
                    bottomCornerCount++;
                    printf("Bottom-right corner at (%d,%d) using tile %d\n", x, y, TILE_BOTTOM_RIGHT_CORNER);
                    break;
                    
                case TILE_WALL:
                    // Interior destructible walls - randomize platform variants
                    gameRenderer->tileVariants[y][x] = TILE_PLATFORM_1 + (rand() % 4);
                    break;
                    
                case TILE_WALL_INNER_TOP_LEFT_CORNER:
                case TILE_WALL_INNER_TOP_RIGHT_CORNER:
                case TILE_WALL_INNER_BOTTOM_LEFT_CORNER:
                case TILE_WALL_INNER_BOTTOM_RIGHT_CORNER:
                    // Inner corners - use platform variants
                    gameRenderer->tileVariants[y][x] = TILE_PLATFORM_1 + (rand() % 4);
                    break;
                    
                case TILE_WALL_INDESTRUCTIBLE:
                    // W tiles (boundary walls) - check neighbors to determine proper tile
                    {
                        // Check if this is actually at the map boundary
                        bool isTopRow = (y == 0);
                        bool isBottomRow = (y == map->height - 1);
                        bool isLeftCol = (x == 0);
                        bool isRightCol = (x == map->width - 1);
                        
                        // For corners, check both boundary conditions
                        if (isTopRow && isLeftCol) {
                            // Top-left: use left wall instead of corner
                            gameRenderer->tileVariants[y][x] = TILE_LEFT_WALL;
                        } else if (isTopRow && isRightCol) {
                            // Top-right: use right wall instead of corner
                            gameRenderer->tileVariants[y][x] = TILE_RIGHT_WALL;
                        } else if (isBottomRow && isLeftCol) {
                            // Bottom-left: use left wall instead of corner
                            gameRenderer->tileVariants[y][x] = TILE_LEFT_WALL;
                        } else if (isBottomRow && isRightCol) {
                            // Bottom-right: use right wall instead of corner
                            gameRenderer->tileVariants[y][x] = TILE_RIGHT_WALL;
                        } else if (isTopRow) {
                            // Top edge - use bottom wall tiles flipped
                            gameRenderer->tileVariants[y][x] = (rand() % 2 == 0) ? TILE_BOTTOM_WALL_1 : TILE_BOTTOM_WALL_2;
                        } else if (isBottomRow) {
                            // Bottom edge
                            gameRenderer->tileVariants[y][x] = (rand() % 2 == 0) ? TILE_BOTTOM_WALL_1 : TILE_BOTTOM_WALL_2;
                            bottomWallCount++;
                            printf("Bottom wall at (%d,%d) using tile %d\n", x, y, gameRenderer->tileVariants[y][x]);
                        } else if (isLeftCol) {
                            // Left edge
                            gameRenderer->tileVariants[y][x] = TILE_LEFT_WALL;
                        } else if (isRightCol) {
                            // Right edge
                            gameRenderer->tileVariants[y][x] = TILE_RIGHT_WALL;
                        } else {
                            // Interior W tile (shouldn't happen in a proper boundary map)
                            gameRenderer->tileVariants[y][x] = TILE_PLATFORM_1 + (rand() % 4);
                        }
                    }
                    break;
                    
                default:
                    // Fallback to background - randomize between 4 variants
                    gameRenderer->tileVariants[y][x] = TILE_BACKGROUND_1 + (rand() % 4);
                    break;
            }
        }
    }
    
    printf("Found %d bottom walls and %d bottom corners\n", bottomWallCount, bottomCornerCount);
    printf("=== Tile Variants Initialized ===\n");
}

static void renderTileFromTileset(SDL_Renderer* renderer, SDL_Texture* tileset, int tileIndex, int columns, SDL_Rect destRect, bool flipVertical) {
    const int TILE_SIZE = 16;
    int srcX = (tileIndex % columns) * TILE_SIZE;
    int srcY = (tileIndex / columns) * TILE_SIZE;
    
    SDL_Rect srcRect = { srcX, srcY, TILE_SIZE, TILE_SIZE };
    
    // Apply vertical flip if requested
    SDL_RendererFlip flip = flipVertical ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, tileset, &srcRect, &destRect, 0.0, NULL, flip);
}

static void renderMap(SDL_Renderer* renderer, SDL_Texture* tileset, int tilesetColumns, Map* map, int** tileVariants) {
    if (!map) return;

    float tileWidth = (float)SCREEN_WIDTH / map->width;
    float tileHeight = (float)SCREEN_HEIGHT / map->height;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            SDL_Rect rect = { (int)(x * tileWidth), (int)(y * tileHeight), (int)tileWidth, (int)tileHeight };
            
            // Get the tile index from the variant array
            int tileIndex = tileVariants[y][x];
            
            // Check if this is a top edge tile that needs to be flipped
            TileType type = map->tile_types[y][x];
            bool flipVertical = (type == TILE_WALL_TOP_EDGE || (type == TILE_WALL_INDESTRUCTIBLE && y == 0));
            
            renderTileFromTileset(renderer, tileset, tileIndex, tilesetColumns, rect, flipVertical);
            
            // Render special overlays for gameplay elements
            switch (type) {
                case TILE_CHECKPOINT:
                    // Draw a green overlay/indicator for checkpoint
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_EXIT:
                    // Draw an orange overlay/indicator for exit
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 150);
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_SPIKE: {
                    // Draw spike indicator (red triangle)
                    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
                    int baseY = (int)((y + 1) * tileHeight);
                    int tipY = (int)(y * tileHeight);
                    int leftX = (int)(x * tileWidth);
                    int rightX = (int)((x + 1) * tileWidth);
                    int centerX = (int)((x + 0.5f) * tileWidth);
                    
                    for (int sy = tipY; sy <= baseY; sy++) {
                        float progress = (float)(sy - tipY) / (baseY - tipY);
                        int lineLeftX = centerX - (int)(progress * (centerX - leftX));
                        int lineRightX = centerX + (int)(progress * (rightX - centerX));
                        SDL_RenderDrawLine(renderer, lineLeftX, sy, lineRightX, sy);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

static void renderPlayer(SDL_Renderer* renderer, Player* player, SDL_Texture* spriteSheet) {
    if (!player || !spriteSheet) return;
    
    // Each sprite is 32x32 pixels in the sprite sheet
    const int spriteSize = 32;
    
    // Calculate source rectangle from sprite sheet
    int frameCount = animFrameCounts[player->currentAnim];
    int srcX = (player->currentFrame % frameCount) * spriteSize;
    int srcY = player->currentAnim * spriteSize;
    
    SDL_Rect srcRect = { srcX, srcY, spriteSize, spriteSize };
    
    // Make sprite width proportional and height match player height
    int renderHeight = (int)player->height;  // Match player/tile height
    int renderWidth = renderHeight;  // Keep square aspect ratio (32x32 sprite)
    
    // Center horizontally on the player hitbox
    int offsetX = (renderWidth - (int)player->width) / 2;
    
    // Position sprite - align bottom with player's bottom, but adjust for death animation
    int spriteY;
    if (player->isDying || player->isRespawning) {
        // During death/respawn, position sprite at the top of the player hitbox
        // so the death animation appears above the ground level
        spriteY = (int)player->y;
    } else {
        // Normal gameplay: align bottom of sprite with player feet
        spriteY = (int)(player->y + player->height - renderHeight);
    }
    
    SDL_Rect destRect = { 
        (int)player->x - offsetX, 
        spriteY,
        renderWidth, 
        renderHeight 
    };
    
    // Flip sprite based on facing direction
    SDL_RendererFlip flip = player->facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    
    SDL_RenderCopyEx(renderer, spriteSheet, &srcRect, &destRect, 0.0, NULL, flip);
}

GameRenderer* initRenderer(const char* title, int width, int height) {
    GameRenderer* gameRenderer = (GameRenderer*)malloc(sizeof(GameRenderer));
    if (!gameRenderer) {
        printf("Failed to allocate GameRenderer\n");
        return NULL;
    }

    gameRenderer->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (!gameRenderer->window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        free(gameRenderer);
        return NULL;
    }

    gameRenderer->renderer = SDL_CreateRenderer(gameRenderer->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gameRenderer->renderer) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gameRenderer->window);
        free(gameRenderer);
        return NULL;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        SDL_DestroyRenderer(gameRenderer->renderer);
        SDL_DestroyWindow(gameRenderer->window);
        free(gameRenderer);
        return NULL;
    }

    // Set texture filtering to nearest neighbor to prevent flickering
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // Load player sprite sheet
    gameRenderer->playerSpriteSheet = loadTexture(gameRenderer->renderer, "Hooded Protagonist Animation Sheet.png");
    if (!gameRenderer->playerSpriteSheet) {
        printf("Failed to load player sprite sheet!\n");
        IMG_Quit();
        SDL_DestroyRenderer(gameRenderer->renderer);
        SDL_DestroyWindow(gameRenderer->window);
        free(gameRenderer);
        return NULL;
    }

    // Load dungeon tileset
    gameRenderer->tilesetTexture = loadTexture(gameRenderer->renderer, "Dungeon_Tileset.png");
    if (!gameRenderer->tilesetTexture) {
        printf("Failed to load dungeon tileset!\n");
        SDL_DestroyTexture(gameRenderer->playerSpriteSheet);
        IMG_Quit();
        SDL_DestroyRenderer(gameRenderer->renderer);
        SDL_DestroyWindow(gameRenderer->window);
        free(gameRenderer);
        return NULL;
    }

    // Query tileset dimensions to calculate rows and columns
    int tilesetWidth, tilesetHeight;
    SDL_QueryTexture(gameRenderer->tilesetTexture, NULL, NULL, &tilesetWidth, &tilesetHeight);
    const int TILE_SIZE = 16;  // Changed from 32 to 16
    gameRenderer->tilesetColumns = tilesetWidth / TILE_SIZE;
    gameRenderer->tilesetRows = tilesetHeight / TILE_SIZE;
    
    printf("Loaded tileset: %dx%d pixels, %d columns x %d rows = %d total tiles (tile size: %dx%d)\n", 
           tilesetWidth, tilesetHeight, 
           gameRenderer->tilesetColumns, gameRenderer->tilesetRows,
           gameRenderer->tilesetColumns * gameRenderer->tilesetRows, TILE_SIZE, TILE_SIZE);

    // Enable alpha blending for transparent sprites
    SDL_SetTextureBlendMode(gameRenderer->playerSpriteSheet, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(gameRenderer->tilesetTexture, SDL_BLENDMODE_BLEND);

    // Initialize random seed for tile variants
    srand((unsigned int)time(NULL));

    return gameRenderer;
}

void renderFrame(GameRenderer* gameRenderer, Map* map, Player* player) {
    if (!gameRenderer) return;

    // Set background color
    SDL_SetRenderDrawColor(gameRenderer->renderer, 25, 25, 40, 255);
    SDL_RenderClear(gameRenderer->renderer);

    // Render game objects
    renderMap(gameRenderer->renderer, gameRenderer->tilesetTexture, gameRenderer->tilesetColumns, map, gameRenderer->tileVariants);
    renderPlayer(gameRenderer->renderer, player, gameRenderer->playerSpriteSheet);

    // Present the frame
    SDL_RenderPresent(gameRenderer->renderer);
}

void renderTilesetTest(GameRenderer* gameRenderer) {
    if (!gameRenderer || !gameRenderer->tilesetTexture) return;

    // Set background color
    SDL_SetRenderDrawColor(gameRenderer->renderer, 40, 40, 50, 255);
    SDL_RenderClear(gameRenderer->renderer);

    // Display tiles in a grid
    const int TILE_SIZE = 16;  // Actual tile size in the tileset
    const int tileDisplaySize = 40; // Display each tile at 40x40 pixels for visibility
    const int padding = 4;
    const int tilesPerRow = 20; // Show 20 tiles per row to fit 100 tiles better
    
    int totalTiles = gameRenderer->tilesetColumns * gameRenderer->tilesetRows;
    
    for (int i = 0; i < totalTiles; i++) {
        int srcX = (i % gameRenderer->tilesetColumns) * TILE_SIZE;
        int srcY = (i / gameRenderer->tilesetColumns) * TILE_SIZE;
        
        int displayX = (i % tilesPerRow) * (tileDisplaySize + padding) + padding;
        int displayY = (i / tilesPerRow) * (tileDisplaySize + padding) + padding + 30; // Leave room for title
        
        // Source rectangle from tileset (16x16)
        SDL_Rect srcRect = { srcX, srcY, TILE_SIZE, TILE_SIZE };
        
        // Destination rectangle on screen (scaled up for visibility)
        SDL_Rect destRect = { displayX, displayY, tileDisplaySize, tileDisplaySize };
        
        // Draw white border around each tile
        SDL_SetRenderDrawColor(gameRenderer->renderer, 255, 255, 255, 255);
        SDL_Rect borderRect = { displayX - 1, displayY - 1, tileDisplaySize + 2, tileDisplaySize + 2 };
        SDL_RenderDrawRect(gameRenderer->renderer, &borderRect);
        
        // Render the tile
        SDL_RenderCopy(gameRenderer->renderer, gameRenderer->tilesetTexture, &srcRect, &destRect);
        
        // Draw tile index number below the tile (we'd need SDL_ttf for actual text)
        // For now, just draw a small indicator
    }
    
    SDL_RenderPresent(gameRenderer->renderer);
}

void destroyRenderer(GameRenderer* gameRenderer) {
    if (!gameRenderer) return;
    if (gameRenderer->tileVariants) {
        for (int y = 0; y < gameRenderer->mapHeight; y++) {
            free(gameRenderer->tileVariants[y]);
        }
        free(gameRenderer->tileVariants);
    }
    if (gameRenderer->tilesetTexture) {
        SDL_DestroyTexture(gameRenderer->tilesetTexture);
    }
    if (gameRenderer->playerSpriteSheet) {
        SDL_DestroyTexture(gameRenderer->playerSpriteSheet);
    }
    if (gameRenderer->renderer) {
        SDL_DestroyRenderer(gameRenderer->renderer);
    }
    if (gameRenderer->window) {
        SDL_DestroyWindow(gameRenderer->window);
    }
    IMG_Quit();
    free(gameRenderer);
}
