#include "renderer.h"
#include "config.h"
#include "tinyutils.h"

void draw_pixel(int x, int y, color_t color, unsigned char* pixels)
{
    pixels[(y * 300 + x) * 4 + 0] = color.r;
    pixels[(y * 300 + x) * 4 + 1] = color.g;
    pixels[(y * 300 + x) * 4 + 2] = color.b;
    pixels[(y * 300 + x) * 4 + 3] = color.a;
}

void render_tile(vec2_t w, vec2_t h, color_t color, unsigned char* pixels, unsigned char* texture, int tw, int th)
{
    float tx = 0, ty = 0;
    float step_tx = tw/(w.y - w.x), step_ty = th/(h.y - h.x);

    for (int y = h.x; y < h.y; y++) 
    {
        tx = 0;
        for (int x = w.x; x < w.y; x++) 
        {
            if (texture)
            {
                int sample_index = ((int)ty * tw + (int)tx);
                color_t color = 
                {
                    texture[sample_index * 4 + 0],
                    texture[sample_index * 4 + 1],
                    texture[sample_index * 4 + 2],
                    texture[sample_index * 4 + 3]
                };
                draw_pixel(x, y, color, pixels);
            }
            else
                draw_pixel(x, y, color, pixels);
            tx += step_tx;
        }
        ty += step_ty;
    }
} 

void renderMap(unsigned char* pixels, Map* map) {
    if (!map) return;

    int tileWidth =  TEXTURE_DEMENSIONS/map->width;
    int tileHeight =  TEXTURE_DEMENSIONS/map->height;

    unsigned char *texture = 0;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            vec2_t w = {x * tileWidth, x * tileWidth + tileWidth};
            vec2_t h = {y * tileHeight, y * tileHeight + tileHeight};
            switch (map->tile_types[y][x]) {
                case TILE_WALL_INDESTRUCTIBLE:
                    render_tile(w, h, {50, 50, 50, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL:
                    render_tile(w, h, {100, 100, 100, 255}, pixels, map->wall_texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_TOP_EDGE:
                    render_tile(w, h, {255, 100, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_BOTTOM_EDGE:
                    render_tile(w, h, {0, 255, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_LEFT_EDGE:
                    render_tile(w, h, {0, 0, 255, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_RIGHT_EDGE:
                    render_tile(w, h, {255, 255, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_TOP_LEFT_CORNER:
                    render_tile(w, h, {255, 0, 255, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_TOP_RIGHT_CORNER:
                    render_tile(w, h, {0, 255, 255, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_BOTTOM_LEFT_CORNER:
                    render_tile(w, h, {255, 128, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_BOTTOM_RIGHT_CORNER:
                    render_tile(w, h, {128, 0, 255, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_WALL_INNER_TOP_LEFT_CORNER:
                case TILE_WALL_INNER_TOP_RIGHT_CORNER:
                case TILE_WALL_INNER_BOTTOM_LEFT_CORNER:
                case TILE_WALL_INNER_BOTTOM_RIGHT_CORNER:
                    render_tile(w, h, {200, 200, 200, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    // color_t col = { 200, 200, 200, 255}; // Light Grey
                    break;
                case TILE_CHECKPOINT:
                    render_tile(w, h, {0, 255, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;                
                case TILE_EXIT:
                    render_tile(w, h, {255, 255, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
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
                            for (int x = leftX; x < lineLeftX; x++)
                                draw_pixel(x, sy, {40, 0, 25, 255}, pixels);
                            for (int x = lineRightX; x < rightX; x++)
                                draw_pixel(x, sy, {40, 0, 25, 255}, pixels);

                            for (int x = lineLeftX; x < lineRightX; x++)
                            {
                                draw_pixel(x, sy, {205, 50, 50, 255}, pixels);
                            }
                            // SDL_RenderDrawLine(renderer, lineLeftX, sy, lineRightX, sy);
                        }
                    }
                    break;
                // Yeah redundency 
                case TILE_SPIKE_BLOODY:
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
                            for (int x = leftX; x < lineLeftX; x++)
                                draw_pixel(x, sy, {40, 0, 25, 255}, pixels);
                            for (int x = lineRightX; x < rightX; x++)
                                draw_pixel(x, sy, {40, 0, 25, 255}, pixels);

                            for (int x = lineLeftX; x < lineRightX; x++)
                            {
                                draw_pixel(x, sy, {255, 0, 0, 255}, pixels);
                            }
                            // SDL_RenderDrawLine(renderer, lineLeftX, sy, lineRightX, sy);
                        }
                    }
                    break;
                case TILE_EMPTY:
                    render_tile(w, h, {40, 0, 25, 255}, pixels, 0, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
                case TILE_PLAYER_START:
                default:
                    render_tile(w, h, {0, 0, 0, 255}, pixels, texture, WALL_SPRITE_X, WALL_SPRITE_Y);
                    break;
            }
        }
    }
}

// void renderPlayer(unsigned char* pixels, Player* player, unsigned char* frame) {
//     if (!player) 
//     {
//         return;
//     }
//     vec2_t playerpos = { (int)player->x, (int)player->x + (int)player->width} ;
//     vec2_t player_dimension = {(int)player->y, (int)player->y + (int)player->height };

//     render_tile(playerpos, player_dimension, {255, 0, 0, 255}, pixels, frame);
// }


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
