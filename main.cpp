
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
#include "player.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

const char *game_name = "Bloody Path";


void findPlayerStart(Map* map, Player* player) {
    if (!map || !player) return;
    float tileWidth = (float)300 / map->width;
    float tileHeight = (float)300 / map->height;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->data[y][x] == 'P') {
                player->x = x * tileWidth;
                player->y = y * tileHeight;
                player->width = tileWidth / 2;
                player->height = tileHeight;
                player->vy = 0;
                player->onGround = false;
                return;
            }
        }
    }
    player->x = 300/2;
    player->y = 300/2;
    player->width = tileWidth/2;
    player->height = tileHeight;
    player->vy = 0;
    player->onGround = false;
}

bool checkWallCollision(float x, float y, Map* map) {
    if (!map) return true; // Treat no map as a solid wall

    float tileWidth = (float)300 / map->width;
    float tileHeight = (float)300 / map->height;

    int mapX = (int)(x / tileWidth);
    int mapY = (int)(y / tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return true; // Collide with boundaries
    }

    char tile = map->data[mapY][mapX];
    return tile == 'W' || tile == '#';
}

bool checkCheckpointCollision(Player* player, Map* map) {
    if (!map) return false;

    float tileWidth = (float)300 / map->width;
    float tileHeight = (float)300 / map->height;

    // Get player center
    float playerCenterX = player->x + player->width / 2;
    float playerCenterY = player->y + player->height / 2;

    int mapX = (int)(playerCenterX / tileWidth);
    int mapY = (int)(playerCenterY / tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return false;
    }

    return map->tile_types[mapY][mapX] == TILE_CHECKPOINT;
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
    canvas.uvs[1] = 0;

    canvas.uvs[2] = 1;
    canvas.uvs[3] = 1;

    canvas.uvs[4] = 0;
    canvas.uvs[5] = 1;

    canvas.uvs[6] = 0;
    canvas.uvs[7] = 0;

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

    int currentLevel = 2;
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

    Player player;
    findPlayerStart(map, &player);
    player.touchedSpike = false;
    player.spikeTimer = 0;

    int mousex, mousey;

    float time = 0;

    const Uint8* keystates = SDL_GetKeyboardState(NULL);

    // generete framebuffer here for post processing effects
    glGenFramebuffers(1, &canvas.frame_buffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas.frame_buffer_id);
    glGenTextures(1, &canvas.frame_buffer_texture);  
    glBindTexture(GL_TEXTURE_2D, canvas.frame_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 300, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas.frame_buffer_texture, 0);

    glGenRenderbuffers(1, &canvas.render_buffer_object);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, TEXTURE_DEMENSIONS, TEXTURE_DEMENSIONS);
    glBindRenderbuffer(GL_RENDERBUFFER, canvas.render_buffer_object);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, canvas.render_buffer_object);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Error creating framebuffer\n");

    while (1) {
        int w, h;
        SDL_GetMouseState(&mousex, &mousey);
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, canvas.frame_buffer_id);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_Event ev;

        time += 0.1;

        renderMap(pixels, map);
        renderPlayer(pixels, &player);

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
                    case SDL_SCANCODE_UP:
                    case SDL_SCANCODE_W:
                        if (player.onGround) {
                            player.vy = JUMP_STRENGTH;
                            player.onGround = false;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

// --- Horizontal Movement ---
        float nextX = player.x;
        if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {
            nextX -= PLAYER_SPEED;
        }
        if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
            nextX += PLAYER_SPEED;
        }

        // Horizontal collision
        if (nextX > player.x) { // Moving right
            if (!checkWallCollision(nextX + player.width, player.y, map) && !checkWallCollision(nextX + player.width, player.y + player.height - 1, map)) {
                player.x = nextX;
            }
        } else if (nextX < player.x) { // Moving left
            if (!checkWallCollision(nextX, player.y, map) && !checkWallCollision(nextX, player.y + player.height - 1, map)) {
                player.x = nextX;
            }
        }

        // --- Vertical Movement (Gravity) ---
        player.vy += GRAVITY;
        float nextY = player.y + player.vy;

        player.onGround = false; // Assume not on ground until proven otherwise

        if (player.vy > 0) { // Moving down
            if (checkWallCollision(player.x, nextY + player.height, map) || checkWallCollision(player.x + player.width - 1, nextY + player.height, map)) {
                // Snap to ground
                float tileHeight = (float)300 / map->height;
                player.y = (int)((nextY + player.height) / tileHeight) * tileHeight - player.height;
                player.vy = 0;
                player.onGround = true;
            } else {
                player.y = nextY;
            }
        } else if (player.vy < 0) { // Moving up
            if (checkWallCollision(player.x, nextY, map) || checkWallCollision(player.x + player.width - 1, nextY, map)) {
                player.vy = 0;
            } else {
                player.y = nextY;
            }
        }

        SDL_SetWindowFullscreen(window, win_mode);
        render_quad_screen(canvas);
        GLint mouse = glGetUniformLocation(canvas.shader_program, "mouse");
        GLint time_u = glGetUniformLocation(canvas.shader_program, "time");
        GLint player_position = glGetUniformLocation(canvas.shader_program, "player_position");
        GLint resolution = glGetUniformLocation(canvas.shader_program, "resolution");
        info_log_shader(mouse);
        info_log_shader(resolution);
        info_log_shader(player_position);


        glUniform2f(player_position, (float)player.x/((float)300/2) - 1, -(float)player.y/((float)300/2) + 1);
        glUniform2f(mouse, (float)mousex/((float)w/2) - 1, -(float)mousey/((float)h/2) + 1);
        glUniform2f(resolution, w, h);
        glUniform1f(time_u, time);


        // post process pass
        glBindFramebuffer(GL_FRAMEBUFFER, canvas.frame_buffer_id);
        glBindTexture(GL_TEXTURE_2D, canvas.texture);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}