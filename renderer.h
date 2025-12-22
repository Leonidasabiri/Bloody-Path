#ifndef RENDERER_H
#define RENDERER_H

#define TEXTURE_DEMENSIONS 300

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "map_parser.h"
#include "player.h"

// Forward-declare the Player struct from main.cpp to avoid circular dependencies



typedef struct
{
    int r, g, b, a;
} color_t;


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
    GLuint  post_process_shader;
    GLuint  post_process_shader_program;
    GLuint  texture;
    GLuint  frame_buffer_texture;
    GLuint  frame_buffer_id;
    GLuint  render_buffer_object;
    char    *buffer;
    int     texture_dimensions;
    int     width;
    int     height;
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
void renderMap(unsigned char* pixels, Map* map);
void draw_pixel(int x, int y, color_t color, unsigned char* pixels);
void renderPlayer(unsigned char* pixels, Player* player, unsigned char*);

#endif // RENDERER_H
