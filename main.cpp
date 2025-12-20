#include "SDL.h"
#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "map_parser.h"
#include "renderer.h"
#include "player.h"

const char *game_name = "Bloody Path";

typedef enum
{
    window_normal  = SDL_WINDOW_SHOWN,
    window_full_screen  = SDL_WINDOW_FULLSCREEN_DESKTOP
}  windowmode_t;

typedef struct
{
    float r, g, b, a;
} color_t;

void draw_pixel(int x, int y, int* buffer, int width, int height, color_t color)
{
    if (x < 0 || y < 0 || x >= width || y > height)
        return;
    int pos = (x + y * width);
    buffer[pos] = (int)color.r << 24 | (int)color.g << 16 | (int)color.b << 8 | (int)color.a;
}

void findPlayerStart(Map* map, Player* player) {
    if (!map || !player) return;
    float tileWidth = (float)SCREEN_WIDTH / map->width;
    float tileHeight = (float)SCREEN_HEIGHT / map->height;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->data[y][x] == 'P') {
                player->x = x * tileWidth;
                player->y = y * tileHeight;
                player->width = tileWidth * 1.5f; // Make player 1.5x wider than tile
                player->height = tileHeight * 1.5f; // Make player 1.5x taller than tile
                player->vy = 0;
                player->onGround = false;
                // Initialize checkpoint to starting position
                player->checkpointX = (float)x;
                player->checkpointY = (float)y;
                return;
            }
        }
    }
    // Default position if 'P' is not found
    player->x = SCREEN_WIDTH / 2;
    player->y = SCREEN_HEIGHT / 2;
    player->width = tileWidth * 1.5f;
    player->height = tileHeight * 1.5f;
    player->vy = 0;
    player->onGround = false;
    player->checkpointX = player->x / tileWidth;
    player->checkpointY = player->y / tileHeight;
}

bool checkWallCollision(float x, float y, Map* map) {
    if (!map) return true; // Treat no map as a solid wall

    float tileWidth = (float)SCREEN_WIDTH / map->width;
    float tileHeight = (float)SCREEN_HEIGHT / map->height;

    int mapX = (int)(x / tileWidth);
    int mapY = (int)(y / tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return true; // Collide with boundaries
    }

    char tile = map->data[mapY][mapX];
    return tile == 'W' || tile == '#';
}

bool checkExitCollisionLocal(Player* player, Map* map) {
    if (!map) return false;

    float tileWidth = (float)SCREEN_WIDTH / map->width;
    float tileHeight = (float)SCREEN_HEIGHT / map->height;

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

bool checkSpikeCollision(Player* player, Map* map) {
    if (!map) return false;

    float tileWidth = (float)SCREEN_WIDTH / map->width;
    float tileHeight = (float)SCREEN_HEIGHT / map->height;

    // Get player center
    float playerCenterX = player->x + player->width / 2;
    float playerCenterY = player->y + player->height / 2;

    int mapX = (int)(playerCenterX / tileWidth);
    int mapY = (int)(playerCenterY / tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return false;
    }

    return map->tile_types[mapY][mapX] == TILE_SPIKE;
}

int main(int argc, char* argv[]) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    GameRenderer* gameRenderer = initRenderer(game_name, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!gameRenderer) {
        SDL_Quit();
        return 1;
    }
    
    // Initialize tile variants pointer
    gameRenderer->tileVariants = NULL;
    gameRenderer->mapWidth = 0;
    gameRenderer->mapHeight = 0;

    windowmode_t win_mode = window_normal;
    int currentLevel = 1;
    Map* map = NULL;

    auto loadLevel = [&](int level) {
        if (map) {
            destroyMap(map);
        }
        char mapPath[256];
        snprintf(mapPath, sizeof(mapPath), "maps/%d.mp", level);
        map = loadMap(mapPath);
        return map != NULL;
    };

    if (!loadLevel(currentLevel)) {
        printf("Failed to load initial level.\n");
        destroyRenderer(gameRenderer);
        SDL_Quit();
        return 1;
    }
    
    // Initialize tile variants for the first level
    initializeTileVariants(gameRenderer, map);

    Player player;
    findPlayerStart(map, &player);
    player.touchedSpike = false;
    player.spikeTimer = 0;
    
    // Initialize animation state
    player.currentAnim = ANIM_IDLE;
    player.currentFrame = 0;
    player.lastFrameTime = SDL_GetTicks();
    player.frameDelay = 200; // Changed from 150ms to 200ms for slower animation
    player.facingRight = true;
    player.isDying = false;
    player.isWaitingToRespawn = false;
    player.isRespawning = false;
    player.deathAnimStartTime = 0;
    player.deathStartX = 0;
    player.deathStartY = 0;
    player.travelStartTime = 0;

    const Uint8* keystates = SDL_GetKeyboardState(NULL);

    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                goto cleanup; // Use goto to ensure all cleanup code is run
            if (ev.type == SDL_KEYDOWN)
            {
                switch (ev.key.keysym.scancode)
                {
                    case SDL_SCANCODE_F:
                        win_mode = (win_mode == window_normal) ? window_full_screen : window_normal;
                        break;    
                    case SDL_SCANCODE_ESCAPE:
                        goto cleanup;
                    case SDL_SCANCODE_UP:
                    case SDL_SCANCODE_W:
                        if (player.onGround) {
                            player.vy = JUMP_STRENGTH;
                            player.onGround = false;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        // --- Horizontal Movement ---
        float nextX = player.x;
        bool isMoving = false;
        
        // Only allow movement if not dying
        if (!player.isDying) {
            if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {
                nextX -= PLAYER_SPEED;
                player.facingRight = false;
                isMoving = true;
            }
            if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
                nextX += PLAYER_SPEED;
                player.facingRight = true;
                isMoving = true;
            }

            // Horizontal collision
            if (nextX > player.x) { // Moving right
                if (!checkWallCollision(nextX + player.width, player.y, map) && !checkWallCollision(nextX + player.width, player.y + player.height - 1, map)) {
                    player.x = nextX;
                }
            } else if (nextX < player.x) { // Moving left
                if (!checkWallCollision(nextX, player.y, map) && !checkWallCollision(nextX, player.y + player.height - 1, map)) {
                    player.x = nextX;
                }
            }
        }

        // --- Vertical Movement (Gravity) ---
        // Only apply gravity if not dying
        if (!player.isDying) {
            player.vy += GRAVITY;
            float nextY = player.y + player.vy;

            player.onGround = false; // Assume not on ground until proven otherwise

            if (player.vy > 0) { // Moving down
                if (checkWallCollision(player.x, nextY + player.height, map) || checkWallCollision(player.x + player.width - 1, nextY + player.height, map)) {
                    // Snap to ground
                    float tileHeight = (float)SCREEN_HEIGHT / map->height;
                    player.y = (int)((nextY + player.height) / tileHeight) * tileHeight - player.height;
                    player.vy = 0;
                    player.onGround = true;
                } else {
                    player.y = nextY;
                }
            } else if (player.vy < 0) { // Moving up
                if (checkWallCollision(player.x, nextY, map) || checkWallCollision(player.x + player.width - 1, nextY, map)) {
                    player.vy = 0;
                } else {
                    player.y = nextY;
                }
            }
        }

        SDL_SetWindowFullscreen(gameRenderer->window, win_mode);

        // Calculate tile dimensions (needed for multiple checks below)
        float tileWidth = (float)SCREEN_WIDTH / map->width;
        float tileHeight = (float)SCREEN_HEIGHT / map->height;

        // Check for spike collision and start timer
        if (checkSpikeCollision(&player, map)) {
            if (!player.touchedSpike) {
                // First time touching spike - start the timer
                player.touchedSpike = true;
                player.spikeTimer = SDL_GetTicks();
            }
        }

        // Check if 10 seconds have passed since touching spike
        if (player.touchedSpike && !player.isDying && !player.isWaitingToRespawn && !player.isRespawning) {
            Uint32 elapsedTime = SDL_GetTicks() - player.spikeTimer;
            float seconds = elapsedTime / 1000.0f;
            
            if (seconds >= 10.0f) {
                // 10 seconds have passed - start death animation
                player.isDying = true;
                player.isWaitingToRespawn = true;
                player.currentAnim = ANIM_DIE;
                player.currentFrame = 0;
                player.lastFrameTime = SDL_GetTicks();
                player.deathAnimStartTime = SDL_GetTicks();
                // Store starting position (death stays at this location)
                player.deathStartX = player.x;
                player.deathStartY = player.y;
            }
        }
        
        // Handle death animation, travel, and respawn sequence
        if (player.isWaitingToRespawn && !player.isRespawning) {
            Uint32 deathAnimElapsed = SDL_GetTicks() - player.deathAnimStartTime;
            // Death animation has 8 frames at 150ms each = 1200ms total
            const Uint32 DEATH_ANIM_DURATION = 8 * 150; // 8 frames * 150ms per frame
            
            // Stay at death position during death animation
            player.x = player.deathStartX;
            player.y = player.deathStartY;
            
            if (deathAnimElapsed >= DEATH_ANIM_DURATION) {
                // Death animation complete - start traveling last frame to checkpoint
                player.isWaitingToRespawn = false;
                player.travelStartTime = SDL_GetTicks();
                // Keep last frame of death animation (frame 7)
                player.currentFrame = 7;
            }
        }
        
        // Travel the last death frame to respawn location
        if (!player.isWaitingToRespawn && !player.isRespawning && player.isDying && player.travelStartTime > 0) {
            Uint32 travelElapsed = SDL_GetTicks() - player.travelStartTime;
            const Uint32 TRAVEL_DURATION = 800; // 800ms to travel to checkpoint
            
            // Smoothly interpolate position to respawn point
            float progress = (float)travelElapsed / (float)TRAVEL_DURATION;
            if (progress > 1.0f) progress = 1.0f;
            
            // Calculate target respawn position
            // Position player so feet are at the bottom of the checkpoint tile
            float targetX = player.checkpointX * tileWidth;
            float targetY = (player.checkpointY + 1) * tileHeight - player.height;
            
            // Interpolate position (ease-in-out for smoother motion)
            float easedProgress = progress * progress * (3.0f - 2.0f * progress); // Smoothstep
            player.x = player.deathStartX + (targetX - player.deathStartX) * easedProgress;
            player.y = player.deathStartY + (targetY - player.deathStartY) * easedProgress;
            
            // Keep showing last frame of death animation during travel
            player.currentFrame = 7;
            
            if (travelElapsed >= TRAVEL_DURATION) {
                // Travel complete - start respawn animation (play death backwards)
                player.isRespawning = true;
                player.currentFrame = 7; // Start from last frame
                player.lastFrameTime = SDL_GetTicks();
                player.travelStartTime = 0;
                
                // Ensure we're exactly at checkpoint position
                player.x = targetX;
                player.y = targetY;
            }
        }
        
        // Play respawn animation (death animation backwards)
        if (player.isRespawning) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsedTime = currentTime - player.lastFrameTime;
            
            if (elapsedTime >= player.frameDelay) {
                player.lastFrameTime = currentTime;
                
                // Go backwards through death animation frames
                player.currentFrame--;
                
                if (player.currentFrame < 0) {
                    // Respawn animation complete - return to normal gameplay
                    
                    // Reset all death and respawn flags
                    player.touchedSpike = false;
                    player.spikeTimer = 0;
                    player.isDying = false;
                    player.isRespawning = false;
                    player.deathAnimStartTime = 0;
                    player.deathStartX = 0;
                    player.deathStartY = 0;
                    
                    // Return to idle animation
                    player.currentAnim = ANIM_IDLE;
                    player.currentFrame = 0;
                    player.lastFrameTime = SDL_GetTicks();
                    player.vy = 0;
                    player.onGround = false;
                }
            }
        }

        // Check for checkpoint collision and save respawn point
        float playerCenterX = player.x + player.width / 2;
        float playerCenterY = player.y + player.height / 2;
        int gridX = (int)(playerCenterX / tileWidth);
        int gridY = (int)(playerCenterY / tileHeight);
        
        // Activate checkpoint if player touches one
        if (::checkCheckpointCollision(map, gridX, gridY, &player.checkpointX, &player.checkpointY)) {
        }

        // Check if player reached the exit (level completion)
        if (checkExitCollisionLocal(&player, map)) {
            currentLevel++;
            if (loadLevel(currentLevel)) {
                initializeTileVariants(gameRenderer, map); // Initialize tile variants for new level
                findPlayerStart(map, &player); // Reset player position for new level
                // Reset animation state for new level
                player.currentAnim = ANIM_IDLE;
                player.currentFrame = 0;
                player.lastFrameTime = SDL_GetTicks();
                player.facingRight = true;
                player.isDying = false;
                player.isWaitingToRespawn = false;
                player.isRespawning = false;
                player.deathAnimStartTime = 0;
                player.deathStartX = 0;
                player.deathStartY = 0;
                player.travelStartTime = 0;
                // Reset spike timer when entering new level
                player.touchedSpike = false;
                player.spikeTimer = 0;
            } else {
                goto cleanup;
            }
        } else {
            // Debug: Check if player is near the exit
            float playerCenterX_exit = player.x + player.width / 2;
            float playerBottomY = player.y + player.height;
            int exitGridX = (int)(playerCenterX_exit / tileWidth);
            int exitGridY = (int)(playerBottomY / tileHeight);
            
            // Only print when player is near the exit area
            if (exitGridX >= 18 && exitGridY >= 10) {
                printf("Player pos: (%.1f, %.1f) size: (%.1f, %.1f) Grid: (%d, %d) Exit at: (%d, %d)\n", 
                       player.x, player.y, player.width, player.height, 
                       exitGridX, exitGridY, map->exit.x, map->exit.y);
            }
        }

        // --- Update Animation State ---
        if (!player.isDying && !player.isRespawning) {
            // Determine which animation to play based on player state
            AnimationType newAnim = ANIM_IDLE;
            
            if (!player.onGround) {
                // Player is in the air - jump animation
                newAnim = ANIM_JUMP;
            } else if (isMoving) {
                // Player is moving on ground - run animation
                newAnim = ANIM_RUN;
            } else {
                // Player is standing still - idle animation
                newAnim = ANIM_IDLE;
            }
            
            // If animation changed, reset frame
            if (newAnim != player.currentAnim) {
                player.currentAnim = newAnim;
                player.currentFrame = 0;
                player.lastFrameTime = SDL_GetTicks();
            }
            
            // Update animation frame based on time with accumulation
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsedTime = currentTime - player.lastFrameTime;
            
            if (elapsedTime >= player.frameDelay) {
                // Calculate how many frames we should advance
                int framesToAdvance = elapsedTime / player.frameDelay;
                
                // Update the last frame time, keeping the remainder for smooth timing
                player.lastFrameTime += framesToAdvance * player.frameDelay;
                
                // Get frame count for current animation
                int frameCount;
                switch (player.currentAnim) {
                    case ANIM_IDLE: frameCount = 2; break;
                    case ANIM_IDLE_BLINK: frameCount = 4; break;
                    case ANIM_WALK: frameCount = 6; break;
                    case ANIM_RUN: frameCount = 8; break;
                    case ANIM_DUCK: frameCount = 4; break;
                    case ANIM_JUMP: frameCount = 4; break;
                    case ANIM_DISAPPEAR: frameCount = 6; break;
                    case ANIM_DIE: frameCount = 8; break;
                    case ANIM_ATTACK: frameCount = 6; break;
                    default: frameCount = 4; break;
                }
                
                // Loop animation
                player.currentFrame = (player.currentFrame + framesToAdvance) % frameCount;
            }
        }
        
        // Update death animation frames (plays forward normally)
        if (player.isDying && !player.isRespawning && player.isWaitingToRespawn) {
            Uint32 currentTime = SDL_GetTicks();
            Uint32 elapsedTime = currentTime - player.lastFrameTime;
            
            if (elapsedTime >= player.frameDelay && player.currentFrame < 7) {
                int framesToAdvance = elapsedTime / player.frameDelay;
                player.lastFrameTime += framesToAdvance * player.frameDelay;
                
                // Advance death animation, stop at last frame
                player.currentFrame += framesToAdvance;
                if (player.currentFrame > 7) { // Ensure we don't go past frame 7
                    player.currentFrame = 7;
                }
            }
        }

        // Rendering is now handled by the renderer module
        renderFrame(gameRenderer, map, &player);
    }

cleanup:
    destroyMap(map);
    destroyRenderer(gameRenderer);
    SDL_Quit();

    return 0;
}