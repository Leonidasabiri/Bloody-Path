#ifndef RENDERER_H
#define RENDERER_H

#define TEXTURE_DEMENSIONS 300

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

#include "map_parser.h"
#include "player.h"

// Forward-declare the Player struct from main.cpp to avoid circular dependencies


typedef enum{
    VERTEX = GL_VERTEX_SHADER, FRAGEMENT = GL_FRAGMENT_SHADER
} shader_type;

typedef struct 
{
    shader_type type;
    GLuint shader_id;
}shader_t;

typedef struct 
{
    float   vertecies[12];
    float   uvs[8];
    float   stride;
    int     indices[6];
    GLuint  vertex_buffer;
    GLuint  vertex_array;
    GLuint  uv_buffer;
    GLuint  indecies_buffer;
    GLuint  shader_program;
    GLuint  vertex_shader;
    GLuint  fragment_shader;
    GLuint  texture;
    GLuint  frame_buffer_id;
    char    *buffer;
    int     texture_dimensions;
}window_canvas_t;

typedef enum
{
    window_normal  = SDL_WINDOW_SHOWN,
    window_full_screen  = SDL_WINDOW_FULLSCREEN_DESKTOP
}  windowmode_t;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    window_canvas_t window_canvas;
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
void renderFrame(GameRenderer gameRenderer, Map* map, Player* player);
/**
 * @brief Destroys the renderer and cleans up resources.
 * 
 * @param gameRenderer The game renderer instance to destroy.
 */
void destroyRenderer(GameRenderer* gameRenderer);

#endif // RENDERER_H
