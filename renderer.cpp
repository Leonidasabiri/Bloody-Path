#include "renderer.h"
#include "config.h"
#include <SDL_image.h>

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

static void renderMap(SDL_Renderer* renderer, Map* map) {
    if (!map) return;

    float tileWidth = (float)SCREEN_WIDTH / map->width;
    float tileHeight = (float)SCREEN_HEIGHT / map->height;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            SDL_Rect rect = { (int)(x * tileWidth), (int)(y * tileHeight), (int)tileWidth, (int)tileHeight };
            switch (map->tile_types[y][x]) {
                case TILE_WALL_INDESTRUCTIBLE:
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark grey
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL:
                    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // Grey
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_TOP_EDGE:
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_BOTTOM_EDGE:
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_LEFT_EDGE:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_RIGHT_EDGE:
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_TOP_LEFT_CORNER:
                    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // Magenta
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_TOP_RIGHT_CORNER:
                    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); // Cyan
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_BOTTOM_LEFT_CORNER:
                    SDL_SetRenderDrawColor(renderer, 255, 128, 0, 255); // Orange
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_BOTTOM_RIGHT_CORNER:
                    SDL_SetRenderDrawColor(renderer, 128, 0, 255, 255); // Purple
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_WALL_INNER_TOP_LEFT_CORNER:
                case TILE_WALL_INNER_TOP_RIGHT_CORNER:
                case TILE_WALL_INNER_BOTTOM_LEFT_CORNER:
                case TILE_WALL_INNER_BOTTOM_RIGHT_CORNER:
                     SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light Grey
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_CHECKPOINT:
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Bright Green for checkpoint
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_EXIT:
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Orange for exit
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case TILE_SPIKE:
                    {
                        // Draw spike as an upward-pointing triangle
                        SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255); // Dark red color for spikes
                        
                        // Define triangle vertices (bottom-left, bottom-right, top-center)
                        int baseY = (int)((y + 1) * tileHeight);  // Bottom of the tile
                        int tipY = (int)(y * tileHeight);          // Top of the tile
                        int leftX = (int)(x * tileWidth);
                        int rightX = (int)((x + 1) * tileWidth);
                        int centerX = (int)((x + 0.5f) * tileWidth);
                        
                        // Draw filled triangle using scanline method
                        for (int sy = tipY; sy <= baseY; sy++) {
                            float progress = (float)(sy - tipY) / (baseY - tipY);
                            int lineLeftX = centerX - (int)(progress * (centerX - leftX));
                            int lineRightX = centerX + (int)(progress * (rightX - centerX));
                            SDL_RenderDrawLine(renderer, lineLeftX, sy, lineRightX, sy);
                        }
                    }
                    break;
                case TILE_EMPTY:
                case TILE_PLAYER_START:
                default:
                    // Do nothing for empty or player start tiles
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

    // Enable alpha blending for transparent sprites
    SDL_SetTextureBlendMode(gameRenderer->playerSpriteSheet, SDL_BLENDMODE_BLEND);

    return gameRenderer;
}

void renderFrame(GameRenderer* gameRenderer, Map* map, Player* player) {
    if (!gameRenderer) return;

    // Set background color
    SDL_SetRenderDrawColor(gameRenderer->renderer, 25, 25, 40, 255);
    SDL_RenderClear(gameRenderer->renderer);

    // Render game objects
    renderMap(gameRenderer->renderer, map);
    renderPlayer(gameRenderer->renderer, player, gameRenderer->playerSpriteSheet);

    // Present the frame
    SDL_RenderPresent(gameRenderer->renderer);
}

void destroyRenderer(GameRenderer* gameRenderer) {
    if (!gameRenderer) return;
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
