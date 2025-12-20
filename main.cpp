
//  [MOUNIR]: We will create a quad the size of the window, then we gonna take a buffer and fill it with our pixels data, 
//  so that we can pass the texture to opengl for post processing effects (multi pass rendering basically with more than just a single framebuffer),
// that's why we have to replace SDL_TEXTURE with our own implementation, to have this controll.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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


const char *game_name = "Bloody Path";


void findPlayerStart(Map* map, Player* player) {
    if (!map || !player) return;
    float tileWidth = (float)TEXTURE_DEMENSIONS / map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS / map->height;

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
    player->x = TEXTURE_DEMENSIONS/2;
    player->y = TEXTURE_DEMENSIONS/2;
    player->width = tileWidth/2;
    player->height = tileHeight;
    player->vy = 0;
    player->onGround = false;
}

bool checkWallCollision(float x, float y, Map* map) {
    if (!map) return true; // Treat no map as a solid wall

    float tileWidth = (float)TEXTURE_DEMENSIONS / map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS / map->height;

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

bool checkSpikeCollision(Player* player, Map* map) {
    if (!map) return false;

    float tileWidth = (float)TEXTURE_DEMENSIONS / map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS / map->height;

    // Get player center
    float playerCenterX = player->x + player->width / 2;
    float playerCenterY = player->y + player->height / 2;

    int mapX = (int)(playerCenterX / tileWidth);
    int mapY = (int)(playerCenterY / tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return false;
    }

    return map->tile_types[mapY][mapX] == TILE_SPIKE;
}

bool checkExitCollisionLocal(Player* player, Map* map) {
    if (!map) return false;

    float tileWidth = (float)TEXTURE_DEMENSIONS / map->width;
    float tileHeight = (float)TEXTURE_DEMENSIONS / map->height;

    // Check if ANY part of the player overlaps with the exit tile
    // Calculate which tiles the player occupies
    // Add a small tolerance (2 pixels) to account for wall collision blocking
    float tolerance = 2.0f;
    int leftTile = (int)(player->x / tileWidth);
    int rightTile = (int)((player->x + player->width + tolerance) / tileWidth);
    int topTile = (int)(player->y / tileHeight);
    int bottomTile = (int)((player->y + player->height - 1) / tileHeight);
    
    // Check all tiles the player overlaps with (or is very close to)
    for (int y = topTile; y <= bottomTile; y++) {
        for (int x = leftTile; x <= rightTile; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                if (map->tile_types[y][x] == TILE_EXIT) {
                    printf("Exit collision detected! Player overlapping exit at grid (%d, %d)\n", x, y);
                    return true;
                }
            }
        }
    }

    return false;
}

shader_t shader(const char* path, shader_type type)
{
    shader_t shader_s;
    FILE *shader_file = fopen(path, "r");

    if (!shader_file)
    {
        perror("Shader not found !!");
        return {};
    }

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
    char infoLog[512];
    if( shaderCompiled != GL_TRUE )
    {
        glGetProgramInfoLog(shader, 512, NULL, infoLog);
        printf( "Unable to compile shader %d! %s\n", shader, infoLog );
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

window_canvas_t window_quad(const char* fragment, const char* vertex, unsigned char* data)
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
    glDeleteShader(canvas.fragment_shader);    

    return canvas;
}

window_canvas_t window_quad_multipass(window_canvas_t canvas, const char* post_process)
{
    // [MOUNIR]: setuping the render pass here
    glGenFramebuffers(1, &canvas.frame_buffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas.frame_buffer_id);
    glViewport(0, 0, canvas.width, canvas.height);
    glGenTextures(1, &canvas.frame_buffer_texture);  
    glBindTexture(GL_TEXTURE_2D, canvas.frame_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas.frame_buffer_texture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Error creating framebuffer\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    canvas.post_process_shader = shader(post_process, FRAGEMENT).shader_id;

    canvas.post_process_shader_program = glCreateProgram();
    glAttachShader(canvas.post_process_shader_program, canvas.vertex_shader);
    glAttachShader(canvas.post_process_shader_program, canvas.post_process_shader);
    glLinkProgram(canvas.post_process_shader_program);

    info_log_shader(canvas.post_process_shader_program);

    return canvas;
}

void render_quad_screen(window_canvas_t canvas_quad, unsigned char *pixels)
{
    glUseProgram(canvas_quad.shader_program);
    glBindTexture(GL_TEXTURE_2D, canvas_quad.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 300, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(canvas_quad.vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void render_quad_post_processing(window_canvas_t canvas_quad)
{
    glUseProgram(canvas_quad.post_process_shader_program);
    glBindTexture(GL_TEXTURE_2D, canvas_quad.frame_buffer_texture);
    glBindVertexArray(canvas_quad.vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

unsigned char  *exctract_sprite_sheet_sample(unsigned char* sprite_sheet, 
                                vec2_t boundsx, 
                                vec2_t boundsy,
                                int texture_width)
{
    int width = -(int)boundsx.x + (int)boundsx.y;
    int height = -(int)boundsy.x + (int)boundsy.y;
    unsigned char* frame = (unsigned char*)malloc(width * height * 4);
    int tx = boundsx.x;
    int ty = boundsy.x;

    for (int y = 0 ; y < height ; y++)
    {
        for (int x = 0 ; x < width; x++)
        {
            frame[(y * width + x) * 4 + 0] = sprite_sheet[((int)ty * texture_width + (int)tx%width) * 4 + 0];
            frame[(y * width + x) * 4 + 1] = sprite_sheet[((int)ty * texture_width + (int)tx%width) * 4 + 1];
            frame[(y * width + x) * 4 + 2] = sprite_sheet[((int)ty * texture_width + (int)tx%width) * 4 + 2];
            frame[(y * width + x) * 4 + 3] = sprite_sheet[((int)ty * texture_width + (int)tx%width) * 4 + 3];
            tx++;
        }
        ty++;
    }
    return frame;
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

    unsigned char* pixels = (unsigned char*)malloc(300 * 300 * 4);

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

    Player player;
    findPlayerStart(map, &player);
    // the whole sprite sheet goes here
    int sprite_w, sprite_h, channels;
    unsigned char* sprite_sheet = stbi_load("assets/Hooded Protagonist Animation Sheet.png", &sprite_w, &sprite_h, &channels, 4);
    player.touchedSpike = false;
    player.spikeTimer = 0;

    window_canvas_t canvas = window_quad("shaders/renderer/pixel/fragment.glsl","shaders/renderer/pixel/vertex.glsl", pixels);
    canvas.width = SCREEN_WIDTH;    
    canvas.height = SCREEN_HEIGHT;
    canvas = window_quad_multipass(canvas, "shaders/post_processing/bloom.glsl");

    int mousex, mousey;

    float time = 0;

    const Uint8* keystates = SDL_GetKeyboardState(NULL);

    unsigned char *frame = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {32 * 3 , 32 * 4}, sprite_w);

    player.animation_frames[0] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w),
    player.animation_frames[1] = exctract_sprite_sheet_sample(sprite_sheet, {32, 64}, {32, 64}, sprite_w);
    player.animation_frames[2] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[3] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[4] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[5] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[6] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[7] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[8] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[9] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[10] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[11] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[12] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[13] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[14] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[15] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[16] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[17] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[18] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[19] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[20] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[21] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[22] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[23] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[24] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[25] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[26] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[27] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[28] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[29] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[30] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[31] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[32] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[33] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[34] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[35] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[36] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[37] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[38] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[39] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[40] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[41] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[42] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[43] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[44] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[45] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[46] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);
    player.animation_frames[47] = exctract_sprite_sheet_sample(sprite_sheet, {0, 32}, {0, 32}, sprite_w);

    while (1) {
        int w, h;
        SDL_GetMouseState(&mousex, &mousey);
        SDL_GetWindowSize(window, &w, &h);

        // [MOUNIR]: first pass here
        glBindFramebuffer(GL_FRAMEBUFFER, canvas.frame_buffer_id);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_Event ev;

        time += 0.1;

        // renderMap(pixels, map);
        // renderPlayer(pixels, &player, frame);
        // test samping
        for (int y = 0 ; y < 32; y++)
        {
            for (int x = 0 ; x < 32; x++)
            {
                int xx = x, yy = y;

                color_t texel = {
                    player.animation_frames[0][(yy * 32 + xx) * 4 + 0],
                    player.animation_frames[0][(yy * 32 + xx) * 4 + 1],
                    player.animation_frames[0][(yy * 32 + xx) * 4 + 2],
                    player.animation_frames[0][(yy * 32 + xx) * 4 + 3]
                };
                if (texel.a != 0)
                 draw_pixel(x, y, texel, pixels);
            }
        }
        render_quad_screen(canvas, pixels);



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
                        canvas.width = w;    
                        canvas.height = h;
                        // canvas =window_quad_multipass(canvas, "shaders/post_processing/bloom.glsl");
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

        // 
        {
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

            if (checkExitCollisionLocal(&player, map)) {
                currentLevel++;
                if (loadLevel(currentLevel)) {
                    findPlayerStart(map, &player); // Reset player position for new level
                    // Reset animation state for new level
                    player.currentAnim = ANIM_IDLE;
                    player.currentFrame = 0;
                    player.lastFrameTime = SDL_GetTicks();
                    player.facingRight = true;
                    player.isDying = false;
                    player.isWaitingToRespawn = false;
                    player.isRespawning = false;
                    player.deathAnimStartTime = 0;
                    player.deathStartX = 0;
                    player.deathStartY = 0;
                    player.travelStartTime = 0;
                    // Reset spike timer when entering new level
                    player.touchedSpike = false;
                    player.spikeTimer = 0;
                }         
            }
        }

        SDL_SetWindowFullscreen(window, win_mode);
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

        // [MOUNIR] : second pass here
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        render_quad_post_processing(canvas);

        resolution = glGetUniformLocation(canvas.post_process_shader_program, "resolution");
        time_u = glGetUniformLocation(canvas.post_process_shader_program, "time");
        info_log_shader(resolution);

        glUniform2f(resolution, w, h);
        glUniform1f(time_u, time);
        // GLint current_program;
        // glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
        // printf("Current program: %d, Expected: %d\n", current_program, canvas.post_process_shader_program);
    

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}