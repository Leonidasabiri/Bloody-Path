#ifndef RENDERER_H
#define RENDERER_H

#include "SDL.h"
#include "map_parser.h"
#include "player.h"

// Forward-declare the Player struct from main.cpp to avoid circular dependencies

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* playerSpriteSheet; // Player sprite sheet texture
} GameRenderer;

/**
 * @brief Initializes the SDL window and renderer.
 * 
 * @param title The title of the window.
 * @param width The width of the window.
 * @param height The height of the window.
 * @return A pointer to the initialized GameRenderer, or NULL on failure.
 */
GameRenderer* initRenderer(const char* title, int width, int height);

/**
 * @brief Renders a single frame of the game.
 * 
 * @param gameRenderer The game renderer instance.
 * @param map The game map to render.
 * @param player The player to render.
 */
void renderFrame(GameRenderer* gameRenderer, Map* map, Player* player);

/**
 * @brief Destroys the renderer and cleans up resources.
 * 
 * @param gameRenderer The game renderer instance to destroy.
 */
void destroyRenderer(GameRenderer* gameRenderer);

#endif // RENDERER_H
