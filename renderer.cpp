#include "renderer.h"
#include "config.h"
#include "tinyutils.h"

void draw_pixel(int x, int y, color_t color, char* pixels)
{
    pixels[(y * 300 + x) * 4 + 0] = color.r;
    pixels[(y * 300 + x) * 4 + 1] = color.g;
    pixels[(y * 300 + x) * 4 + 2] = color.b;
    pixels[(y * 300 + x) * 4 + 3] = color.a;
}

void render_tile(vec2_t w, vec2_t h, color_t color, char* pixels)
{
    for (int y = h.x; y < h.y; y++) {
        for (int x = w.x; x < w.y; x++) {
            draw_pixel(x, y, color, pixels);
        }
    }
} 

void renderMap(char* pixels, Map* map) {
    if (!map) return;

    float tileWidth = (float)TEXTURE_DEMENSIONS/map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS/map->height;

    printf("%f\n", tileWidth);

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            vec2_t w = {x * tileWidth, x * tileWidth + tileWidth};
            vec2_t h = {y * tileHeight, y * tileHeight + tileHeight};
            switch (map->tile_types[y][x]) {
                case TILE_WALL_INDESTRUCTIBLE:
                    render_tile(w, h, {50, 50, 50, 255}, pixels);
                    break;
                case TILE_WALL:
                    render_tile(w, h, {100, 100, 100, 255}, pixels);
                    break;
                case TILE_WALL_TOP_EDGE:
                    render_tile(w, h, {255, 0, 0, 255}, pixels);
                    break;
                case TILE_WALL_BOTTOM_EDGE:
                    render_tile(w, h, {0, 255, 0, 255}, pixels);
                    break;
                case TILE_WALL_LEFT_EDGE:
                    render_tile(w, h, {0, 0, 255, 255}, pixels);
                    break;
                case TILE_WALL_RIGHT_EDGE:
                    render_tile(w, h, {255, 255, 0, 255}, pixels);
                    break;
                case TILE_WALL_TOP_LEFT_CORNER:
                    render_tile(w, h, {255, 0, 255, 255}, pixels);
                    break;
                case TILE_WALL_TOP_RIGHT_CORNER:
                    render_tile(w, h, {0, 255, 255, 255}, pixels);
                    break;
                case TILE_WALL_BOTTOM_LEFT_CORNER:
                    render_tile(w, h, {255, 128, 0, 255}, pixels);
                    break;
                case TILE_WALL_BOTTOM_RIGHT_CORNER:
                    render_tile(w, h, {128, 0, 255, 255}, pixels);
                    break;
                case TILE_WALL_INNER_TOP_LEFT_CORNER:
                case TILE_WALL_INNER_TOP_RIGHT_CORNER:
                case TILE_WALL_INNER_BOTTOM_LEFT_CORNER:
                case TILE_WALL_INNER_BOTTOM_RIGHT_CORNER:
                    render_tile(w, h, {200, 200, 200, 255}, pixels);
                    // color_t col = { 200, 200, 200, 255}; // Light Grey
                    break;
                case TILE_CHECKPOINT:
                    render_tile(w, h, {0, 255, 0, 255}, pixels);
                    // color_t col = { 0, 255, 0, 255}; // Bright Green for checkpoint
                    break;
                case TILE_SPIKE:
                    {
                        // Draw spike as an upward-pointing triangle
                        // SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255); // Dark red color for spikes
                        
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
                            // SDL_RenderDrawLine(renderer, lineLeftX, sy, lineRightX, sy);
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

static void renderPlayer(SDL_Renderer* renderer, Player* player) {
    if (!player) return;
    SDL_Rect playerRect = { (int)player->x, (int)player->y, (int)player->width, (int)player->height };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red placeholder
    SDL_RenderFillRect(renderer, &playerRect);
}


// void renderFrame(GameRenderer gameRenderer, Map* map, Player* player) {

//     // Render game objects
//     renderMap(gameRenderer, map);
//     // renderPlayer(gameRenderer.renderer, player);

//     // Present the frame
//     SDL_RenderPresent(gameRenderer.renderer);
// }

// void destroyRenderer(GameRenderer* gameRenderer) {
//     if (!gameRenderer) return;
//     if (gameRenderer->renderer) {
//         SDL_DestroyRenderer(gameRenderer->renderer);
//     }
//     if (gameRenderer->window) {
//         SDL_DestroyWindow(gameRenderer->window);
//     }
//     free(gameRenderer);
// }
