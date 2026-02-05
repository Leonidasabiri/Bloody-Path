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

enum windowmode_t
{
    window_normal  = SDL_WINDOW_SHOWN,
    window_full_screen  = SDL_WINDOW_FULLSCREEN_DESKTOP
};

struct color_t
{
    int r, g, b, a;
};

struct texture_t
{
    int             texture_width, texture_height, channels;
    unsigned char *texture_data;
};

typedef enum{
    VERTEX = GL_VERTEX_SHADER, FRAGEMENT = GL_FRAGMENT_SHADER
} shader_type;

struct shader_t
{
    shader_type type;
    GLuint shader_id;
};

struct window_canvas_t
{
    float   vertecies[12];
    float   uvs[8];
    float   stride;
    int     indices[6];
    GLuint  vertex_buffer;
    GLuint  vertex_array;
    GLuint  instance_buffer;
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
    int       height;
    int       mousex, mousey;
    float     scale;
    float     global_scale;
    float     opacity;
    texture_t texturee;
    vec2_t    position;
    vec2_t    uv_side;
};

struct particle_t
{
    window_canvas_t  quad;
    texture_t        particle_texture;
    vec2_t           start_position;
    vec2_t           velocity;
    vec2_t           offset_intsances[1000];
    float            particle_delay[1000];
    float            particle_gravity[1000];
    float            emission_speed;
    float            size;
    float            force;
    float            delay;
    float            life_time;
    float            current_time;
    int              instanciated_particles_number;
    void             initiate_particle(vec2_t);
    void             update_particle(vec2_t, float);
    bool             particles_done;
};

shader_t shader(const char* path, shader_type type);

void shader_int_value(GLuint, int, const char* uniform);
void shader_float_value(GLuint, float, const char* uniform);
void shader_float2_value(GLuint, float, float, const char* uniform);

void draw_pixel(int x, int y, color_t color, unsigned char* pixels);
void info_log_shader(GLuint id);
window_canvas_t window_quad(const char* fragment, const char* vertex, unsigned char* data, 
    int w, int h, vec2_t width, vec2_t height);
window_canvas_t window_quad_multipass(window_canvas_t canvas, const char* post_process);
void render_quad(window_canvas_t quad);

void setup_quad_screen(window_canvas_t canvas_quad);
void render_quad_screen(window_canvas_t canvas_quad, int w, int h, bool instanced = 0, int count = 0, void *data = 0);
void render_quad_post_processing(window_canvas_t canvas_quad);

#endif
