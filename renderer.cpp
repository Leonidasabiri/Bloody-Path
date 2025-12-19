#include "renderer.h"
#include "config.h"

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
                case TILE_EMPTY:
                case TILE_PLAYER_START:
                default:
                    // Do nothing for empty or player start tiles
                    break;
            }
        }
    }
}

static void renderPlayer(SDL_Renderer* renderer, Player* player) {
    if (!player) return;
    SDL_Rect playerRect = { (int)player->x, (int)player->y, (int)player->width, (int)player->height };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red placeholder
    SDL_RenderFillRect(renderer, &playerRect);
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

    return gameRenderer;
}

void renderFrame(GameRenderer* gameRenderer, Map* map, Player* player) {
    if (!gameRenderer) return;

    // Set background color
    SDL_SetRenderDrawColor(gameRenderer->renderer, 25, 25, 40, 255);
    SDL_RenderClear(gameRenderer->renderer);

    // Render game objects
    renderMap(gameRenderer->renderer, map);
    renderPlayer(gameRenderer->renderer, player);

    // Present the frame
    SDL_RenderPresent(gameRenderer->renderer);
}

void destroyRenderer(GameRenderer* gameRenderer) {
    if (!gameRenderer) return;
    if (gameRenderer->renderer) {
        SDL_DestroyRenderer(gameRenderer->renderer);
    }
    if (gameRenderer->window) {
        SDL_DestroyWindow(gameRenderer->window);
    }
    free(gameRenderer);
}
