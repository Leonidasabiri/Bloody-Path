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
                player->width = tileWidth / 2;
                player->height = tileHeight;
                player->vy = 0;
                player->onGround = false;
                return;
            }
        }
    }
    // Default position if 'P' is not found
    player->x = SCREEN_WIDTH / 2;
    player->y = SCREEN_HEIGHT / 2;
    player->width = tileWidth / 2;
    player->height = tileHeight;
    player->vy = 0;
    player->onGround = false;
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

bool checkCheckpointCollision(Player* player, Map* map) {
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

    return map->tile_types[mapY][mapX] == TILE_CHECKPOINT;
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

    Player player;
    findPlayerStart(map, &player);
    player.touchedSpike = false;
    player.spikeTimer = 0;

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
        if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {
            nextX -= PLAYER_SPEED;
        }
        if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
            nextX += PLAYER_SPEED;
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

        // --- Vertical Movement (Gravity) ---
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

        SDL_SetWindowFullscreen(gameRenderer->window, win_mode);

        // Check for spike collision and start timer
        if (checkSpikeCollision(&player, map)) {
            if (!player.touchedSpike) {
                // First time touching spike - start the timer
                player.touchedSpike = true;
                player.spikeTimer = SDL_GetTicks();
                printf("Spike touched! Timer started.\n");
            }
        }

        // Update and display timer if spike was touched
        if (player.touchedSpike) {
            Uint32 elapsedTime = SDL_GetTicks() - player.spikeTimer;
            float seconds = elapsedTime / 1000.0f;
            printf("Time since spike touch: %.2f seconds\n", seconds);
        }

        // Rendering is now handled by the renderer module
        renderFrame(gameRenderer, map, &player);

        // Check for checkpoint collision
        if (checkCheckpointCollision(&player, map)) {
            currentLevel++;
            if (loadLevel(currentLevel)) {
                printf("Loading level %d\n", currentLevel);
                findPlayerStart(map, &player); // Reset player position for new level
            } else {
                printf("You win! No more levels.\n");
                goto cleanup;
            }
        }
    }

cleanup:
    destroyMap(map);
    destroyRenderer(gameRenderer);
    SDL_Quit();

    return 0;
}
