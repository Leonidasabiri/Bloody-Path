#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include <GL/gl.h>
// #include <GL/glew.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

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

int main(int argc, char* argv[]) {


    windowmode_t win_mode = window_normal;
    SDL_Window* window = SDL_CreateWindow(game_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, win_mode | SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* surface = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR32, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
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

    void* pixels;
    int   pitch;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    while (1) {
        SDL_Event ev;
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
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
