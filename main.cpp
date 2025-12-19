
//  [MOUNIR]: We will create a quad the size of the window, then we gonna take a buffer and fill it with our pixels data, 
//  so that we can pass the texture to opengl for post processing effects (multi pass rendering basically with more than just a single framebuffer),
// that's why we have to replace SDL_TEXTURE with our own implementation, to have this controll.

#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include <GL/gl.h>
#include "tinyutils.h"
#include "config.h"
#include "map_parser.h"
#include "renderer.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

const char *game_name = "Bloody Path";

typedef struct
{
    float r, g, b, a;
} color_t;


void draw_pixel(int x, int y, color_t color, char* pixels)
{
    pixels[(y * 300 + x) * 4 + 0] = color.r;
    pixels[(y * 300 + x) * 4 + 1] = color.g;
    pixels[(y * 300 + x) * 4 + 2] = color.b;
    pixels[(y * 300 + x) * 4 + 3] = color.a;
}

shader_t shader(const char* path, shader_type type)
{
    shader_t shader_s;
    FILE *shader_file = fopen(path, "r");
    fseek(shader_file, 0, SEEK_END);
    int file_size = ftell(shader_file);
    char *shader_code = (char*)malloc(file_size + 1);
    shader_code[file_size] = 0;
    fseek(shader_file, 0, SEEK_SET);
    fread(shader_code, 1, file_size, shader_file);

    GLuint shader = glCreateShader( type );

    glShaderSource( shader, 1, &shader_code, NULL);
    glCompileShader( shader );
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &shaderCompiled );
    if( shaderCompiled != GL_TRUE )
    {
        printf( "Unable to compile shader %d!\n", shader );
    }

    shader_s.shader_id = shader;
    return shader_s;
}

void info_log_shader(GLuint id)
{
    int success;
    char infoLog[512];
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED %s\n", infoLog);
    }
}

window_canvas_t window_quad(const char* fragment, const char* vertex, char* data)
{
    // [MOUNIR]: Opengl by default works with ndc so playing on the range [-1, 1] would always be mapped to the window borders
    window_canvas_t canvas;

    canvas.vertecies[0] = 1;
    canvas.vertecies[1] = 1;
    canvas.vertecies[2] = 0;    // z always 0

    canvas.vertecies[3] = 1;
    canvas.vertecies[4] = -1;
    canvas.vertecies[5] = 0;    // z always 0

    canvas.vertecies[6] = -1;
    canvas.vertecies[7] = -1;
    canvas.vertecies[8] = 0;    // z always 0

    canvas.vertecies[9] = -1;
    canvas.vertecies[10] = 1;
    canvas.vertecies[11] = 0;    // z always 0

    // 
    canvas.uvs[0] = 1;
    canvas.uvs[1] = 1;

    canvas.uvs[2] = 1;
    canvas.uvs[3] = 0;

    canvas.uvs[4] = 0;
    canvas.uvs[5] = 0;

    canvas.uvs[6] = 0;
    canvas.uvs[7] = 1;

    canvas.indices[0] = 0;
    canvas.indices[1] = 1;
    canvas.indices[2] = 3;

    canvas.indices[3] = 1;
    canvas.indices[4] = 2;
    canvas.indices[5] = 3; 

    canvas.vertex_shader = shader(vertex, VERTEX).shader_id;
    canvas.fragment_shader = shader(fragment, FRAGEMENT).shader_id;

    canvas.stride = 3;

    canvas.shader_program = glCreateProgram();

    // vertecies and indecies drawing
    glGenVertexArrays(1, &canvas.vertex_array);
    glGenBuffers(1, &canvas.vertex_buffer);
    glGenBuffers(1, &canvas.indecies_buffer);

    glBindVertexArray(canvas.vertex_array);

    glBindBuffer(GL_ARRAY_BUFFER, canvas.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(canvas.vertecies), canvas.vertecies, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, canvas.indecies_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(canvas.indices), canvas.indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // textures generation
    glGenTextures(1, &canvas.texture);  
    glBindTexture(GL_TEXTURE_2D, canvas.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 300, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenBuffers(1, &canvas.uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, canvas.uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(canvas.uvs), canvas.uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, canvas.uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // shaders linking
    glAttachShader(canvas.shader_program, canvas.vertex_shader);
    glAttachShader(canvas.shader_program, canvas.fragment_shader);
    glLinkProgram(canvas.shader_program);

    info_log_shader(canvas.shader_program);

    // clean up
    glDeleteShader(canvas.vertex_shader);
    glDeleteShader(canvas.fragment_shader);

    return canvas;
}

void render_quad_screen(window_canvas_t canvas_quad)
{
    glUseProgram(canvas_quad.shader_program);
    glBindVertexArray(canvas_quad.vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
                    // color_t col = { 200, 200, 200, 255}; // Light Grey
                    break;
                case TILE_CHECKPOINT:
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

int main(int argc, char* argv[]) {

    int textw = 100, texth = 100;

    windowmode_t win_mode = window_normal;
    SDL_Window* window = SDL_CreateWindow(game_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, win_mode | SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (!context) {
        printf("SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_MakeCurrent(window, context);

    char* pixels = (char*)malloc(300 * 300 * 4);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    int y = 12, x = 12;

    GLenum glewError = glewInit();
    if( glewError != GLEW_OK )
    {
        printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
    }

    window_canvas_t canvas = window_quad("shaders/renderer/pixel/fragment.glsl","shaders/renderer/pixel/vertex.glsl", pixels);
    draw_pixel(10, 10, {255, 255, 0, 255}, pixels);

    int currentLevel = 1;
    Map* map = NULL;

    auto loadLevel = [&](int level) {
        char mapPath[256];
        snprintf(mapPath, sizeof(mapPath), "maps/%d.mp", level);
        map = loadMap(mapPath);
        return map;
    };

    if (!loadLevel(currentLevel)) {
        printf("Failed to load initial level.\n");
        SDL_Quit();
        return 1;
    }

    renderMap(pixels, map);

    while (1) {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_Event ev;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 300, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                return 0;
            if (ev.type == SDL_KEYDOWN)
            {
                switch (ev.key.keysym.scancode)
                {
                    case SDL_SCANCODE_F:
                        win_mode = (win_mode == window_normal) ? window_full_screen : window_normal;
                        break;    
                    case SDL_SCANCODE_ESCAPE:
                        return 0;  
                    default:
                        break;
                }
            }
        }
        SDL_SetWindowFullscreen(window, win_mode);
        render_quad_screen(canvas);
        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}