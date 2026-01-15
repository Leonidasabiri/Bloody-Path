#ifndef RENDERER_H
#define RENDERER_H

#define TEXTURE_DEMENSIONS 300

#include <stdio.h>
#include <stdbool.h>

#include <glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "../player/player.h"
#include "../../tinyutils.h"

typedef enum
{
    window_normal  = SDL_WINDOW_SHOWN,
    window_full_screen  = SDL_WINDOW_FULLSCREEN_DESKTOP
}  windowmode_t;

typedef struct
{
    int r, g, b, a;
} color_t;

typedef struct
{
    int             texture_width, texture_height, channels;
    unsigned char *texture_data;
} texture_t;

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
    int     mousex, mousey;
    float     scale;
    texture_t texturee;
    vec2_t  position;
}window_canvas_t;

void draw_pixel(int x, int y, color_t color, unsigned char* pixels);
shader_t shader(const char* path, shader_type type);
void info_log_shader(GLuint id);
window_canvas_t window_quad(const char* fragment, const char* vertex, unsigned char* data, int w, int h, vec2_t width, vec2_t height);
window_canvas_t window_quad_multipass(window_canvas_t canvas, const char* post_process);

void render_quad(window_canvas_t quad);

void render_quad_screen(window_canvas_t canvas_quad);
void render_quad_post_processing(window_canvas_t canvas_quad);

#endif
