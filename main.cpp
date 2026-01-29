//  [MOUNIR]: We will create a quad the size of the window, then we gonna take a buffer and fill it with our pixels data, 
//  so that we can pass the texture to opengl for post processing effects (multi pass rendering basically with more than just a single framebuffer),
// that's why we have to replace SDL_TEXTURE with our own implementation, to have this controll.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "tinyutils.h"
#include "config.h"
#include "engine/gui/imgui.h"
#include "engine/gui/imgui_impl_sdl2.h"
#include "engine/gui/imgui_impl_opengl3.h"
#include "engine/tools/map_parser.h"
#include "engine/rendering/renderer.h"
#include "player/player.h"
#include <al.h>
#include <alc.h>
#include <time.h>

const char *game_name = "Bloody Path";

void findPlayerStart(Map* map, Player* player) {
    if (!map || !player) return;
    float tileWidth = WALL_SPRITE_X;
    float tileHeight = WALL_SPRITE_X;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->data[y][x] == 'P') {
                player->x = x * WALL_SPRITE_X/((float)SCREEN_WIDTH/2);
                player->y = (map->height - y) * WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);
                player->width = (WALL_SPRITE_X-10)/((float)SCREEN_WIDTH/2);
                player->height = (WALL_SPRITE_X)/((float)SCREEN_HEIGHT/2);
                player->vy = 0;
                player->onGround = false;
                player->checkpointX = (float)x;
                player->checkpointY = (float)y;
                return;
            }
        }
    }
    player->x = SCREEN_WIDTH/2;
    player->y = SCREEN_HEIGHT/2;
    player->width = WALL_SPRITE_X/2;
    player->height = WALL_SPRITE_X;
    player->vy = 0;
    player->onGround = false;
    player->checkpointX = player->x / WALL_SPRITE_X;
    player->checkpointY = player->y / WALL_SPRITE_X;
}

bool checkWallCollision(float x, float y, Map* map) {
    if (!map) return true; // Treat no map as a solid wall

    float tileWidth = (float)WALL_SPRITE_X/((float)SCREEN_WIDTH/2);
    float tileHeight = (float)WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);

    int mapX = (int)(x/tileWidth);
    int mapY = map->height - (int)(y/tileHeight);

    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return true; // Collide with boundaries
    }

    char tile = map->data[mapY][mapX];
    return tile == 'W' || tile == '#';
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

    if (map->tile_types[mapY][mapX] == TILE_SPIKE || map->tile_types[mapY][mapX] == TILE_SPIKE_BLOODY)
    {
        map->tile_types[mapY][mapX] = TILE_SPIKE_BLOODY;
    }
    return map->tile_types[mapY][mapX] == TILE_SPIKE_BLOODY;
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
            frame[(y * width + x) * 4 + 0] = sprite_sheet[((int)ty * texture_width + (int)tx) * 4 + 0];
            frame[(y * width + x) * 4 + 1] = sprite_sheet[((int)ty * texture_width + (int)tx) * 4 + 1];
            frame[(y * width + x) * 4 + 2] = sprite_sheet[((int)ty * texture_width + (int)tx) * 4 + 2];
            frame[(y * width + x) * 4 + 3] = sprite_sheet[((int)ty * texture_width + (int)tx) * 4 + 3];
            if (tx >= boundsx.y) tx = boundsx.x;
            tx++;
        }
        ty++;
    }
    return frame;
}


// Batch the whole scene/level geometry in this function
// window_canvas_t setup_scene(Map *map)
// {
// 	window_canvas_t scene;

// 	// scene.vertecies
// 	for (int y = 0; y < map->height + 0 ; ++y)
// 	{
// 		for (int x = 0; x < map->width + 0 ; ++x)
// 		{
// 			if (map->tile_types[y][x] != TILE_EMPTY)
// 			{

// 			}
// 		}
// 	}
// }

void render_scene(Map *map, window_canvas_t quad, float w, float h, vec2_t offset)
{

	for (int y = 0; y < map->height + 0 ; ++y)
	{
		for (int x = 0; x < map->width + 0 ; ++x)
		{
			switch (map->tile_types[y][x])
			{
				case TILE_WALL_INDESTRUCTIBLE:
					quad.texturee.texture_data   = map->wall_texture.texture_data;
					quad.texturee.texture_width  = map->wall_texture.texture_width;
					quad.texturee.texture_height = map->wall_texture.texture_height;
					break;
				case TILE_WALL_LEFT_EDGE:
					quad.texturee.texture_data   = map->left_wall_texture.texture_data;
					quad.texturee.texture_width  = map->left_wall_texture.texture_width;
					quad.texturee.texture_height = map->left_wall_texture.texture_height;
					break;
				case TILE_WALL_RIGHT_EDGE:
					quad.texturee.texture_data   = map->right_wall_texture.texture_data;
					quad.texturee.texture_width  = map->right_wall_texture.texture_width;
					quad.texturee.texture_height = map->right_wall_texture.texture_height;
					break;
				case TILE_SPIKE:
					quad.texturee.texture_data   = map->spike_texture.texture_data;
					quad.texturee.texture_width  = map->spike_texture.texture_width;
					quad.texturee.texture_height = map->spike_texture.texture_height;
				case TILE_EMPTY:
					quad.texturee.texture_data   = map->empty_texture.texture_data;
					quad.texturee.texture_width  = map->empty_texture.texture_width;
					quad.texturee.texture_height = map->empty_texture.texture_height;
					break;
				case TILE_WALL_TOP_LEFT_CORNER:
					quad.texturee.texture_data   = map->top_left_wall_texture.texture_data;
					quad.texturee.texture_width  = map->top_left_wall_texture.texture_width;
					quad.texturee.texture_height = map->top_left_wall_texture.texture_height;
					break;
				case TILE_WALL_BOTTOM_LEFT_CORNER:
					quad.texturee.texture_data   = map->bottom_left_wall_texture.texture_data;
					quad.texturee.texture_width  = map->bottom_left_wall_texture.texture_width;
					quad.texturee.texture_height = map->bottom_left_wall_texture.texture_height;
					break;
				default:
					break;
			}
			{
				quad.position.x = w/((float)SCREEN_WIDTH/2) * x + offset.x/((float)SCREEN_WIDTH/2);
				quad.position.y = h/((float)SCREEN_HEIGHT/2) * (map->height - y) + offset.y/((float)SCREEN_HEIGHT/2);
				if (map->tile_types[y][x] != TILE_EMPTY)
					glStencilMask(0xFF);
				else
					glStencilMask(0x0);
				render_quad_screen(quad);
			}
		}
	}
}

int main(int argc, char* argv[]) {
	(void)argc;  // Suppress unused parameter warning
	(void)argv;  // Suppress unused parameter warning

	// Initialize SDL first
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	// Set OpenGL attributes BEFORE creating the window
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	}

	ALCdevice *device = alcOpenDevice(0);
	const ALCchar* device_name = alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER);

	if (device_name)
	{
		ALCcontext *context = alcCreateContext(device, 0);
	}

	printf("%s\n", device_name);

	// Create window with OpenGL flag
	windowmode_t win_mode = window_normal;
	SDL_Window* window = SDL_CreateWindow(game_name,
											SDL_WINDOWPOS_UNDEFINED,
											SDL_WINDOWPOS_UNDEFINED,
											SCREEN_WIDTH,
											SCREEN_HEIGHT,
											SDL_WINDOW_OPENGL | win_mode);


	if (!window) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// Create OpenGL context
	SDL_GLContext context = SDL_GL_CreateContext(window);
	if (!context) {
		printf("SDL_GL_CreateContext error: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Make the context current
	SDL_GL_MakeCurrent(window, context);

	// Enable VSync
	SDL_GL_SetSwapInterval(1);

	// Initialize GLEW
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	int currentLevel = 1;
	Map* map = NULL;
	
	int sprite_w, sprite_h, channels;
	int tiles_sprite_w, tiles_sprite_h, tiles_sprite_channels;
	auto loadLevel = [&](int level) {
		char mapPath[256];
		snprintf(mapPath, sizeof(mapPath), "maps/%d.mp", level);
		map = loadMap(mapPath);
		return map;
	};
	
	if (!loadLevel(currentLevel)) {
		printf("Failed to load initial level.\n");
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	
	map->spike_texture.texture_data = stbi_load("assets/spike.png", &map->spike_texture.texture_width, 
																	&map->spike_texture.texture_height, 
																	&map->spike_texture.channels, 4);


    ImGui::CreateContext();
	ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330");

	ImGui::StyleColorsDark();

	Player player;
	findPlayerStart(map, &player);
	// the whole sprite sheet goes here
	unsigned char* sprite_sheet = stbi_load("assets/Hooded Protagonist Animation Sheet.png", &sprite_w, &sprite_h, &channels, 4);
	unsigned char* tiles_sprite = stbi_load("assets/Dungeon_Tileset.png", &tiles_sprite_w, &tiles_sprite_h, &tiles_sprite_channels, 4);


	{
		map->wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 1, 16 * 2}, {16 * 4, 16 * 5}, tiles_sprite_w);
		map->wall_texture.texture_width = 16;
		map->wall_texture.texture_height = 16;

		map->left_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 5, 16 * 6}, {16 * 0, 16 * 1}, tiles_sprite_w);
		map->left_wall_texture.texture_width = 16;
		map->left_wall_texture.texture_height = 16;

		map->right_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 0, 16 * 1}, tiles_sprite_w);
		map->right_wall_texture.texture_width = 16;
		map->right_wall_texture.texture_height = 16;

		map->empty_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 1, 16 * 2}, {16 * 1, 16 * 2}, tiles_sprite_w);
		map->empty_texture.texture_width = 16;
		map->empty_texture.texture_height = 16;
		
		map->top_left_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 5, 16 * 6}, tiles_sprite_w);
		map->top_left_wall_texture.texture_width = 16;
		map->top_left_wall_texture.texture_height = 16;

		map->bottom_left_wall_texture.texture_data = exctract_sprite_sheet_sample(tiles_sprite, {16 * 0, 16 * 1}, {16 * 4, 16 * 5}, tiles_sprite_w);
		map->bottom_left_wall_texture.texture_width = 16;
		map->bottom_left_wall_texture.texture_height = 16;
	}
	player.touchedSpike = false;
	player.spikeTimer = 0;

	float s = time(NULL);
	float time = 0;

	const Uint8* keystates = SDL_GetKeyboardState(NULL);

	{
		// Initialize animation state
		player.currentAnim = ANIM_IDLE;
		player.currentFrame = 0;
		player.lastFrameTime = SDL_GetTicks();
		player.frameDelay = 200; // Changed from 150ms to 200ms for slower animation
		player.facingRight = true;
		player.isDying = false;
		player.isWaitingToRespawn = false;
		player.isRespawning = false;
		player.deathAnimStartTime = 0;
		player.deathStartX = 0;
		player.deathStartY = 0;
		player.max_health = 30;
		player.health = player.max_health;
		player.travelStartTime = 0;
	}

	{
		player.idle_frames[0] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 0, 32 * 1}, {32 * 0, 32 * 1}, sprite_w),
		player.idle_frames[1] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 1, 32 * 2}, {32 * 0, 32 * 1}, sprite_w);
		player.idle_frames[2] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 0, 32 * 1}, {32 * 1, 32 * 2}, sprite_w);
		player.idle_frames[3] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 1, 32 * 2}, {32 * 1, 32 * 2}, sprite_w);

		player.death_frames[0] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 0, 32 * 1}, {32 * 7, 32 * 8}, sprite_w),
		player.death_frames[1] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 1, 32 * 2}, {32 * 7, 32 * 8}, sprite_w);
		player.death_frames[2] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 2, 32 * 3}, {32 * 7, 32 * 8}, sprite_w);
		player.death_frames[3] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 3, 32 * 4}, {32 * 7, 32 * 8}, sprite_w);
		player.death_frames[4] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 4, 32 * 5}, {32 * 7, 32 * 8}, sprite_w),
		player.death_frames[5] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 5, 32 * 6}, {32 * 7, 32 * 8}, sprite_w);
		player.death_frames[6] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 6, 32 * 7}, {32 * 7, 32 * 8}, sprite_w);
		player.death_frames[7] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 7, 32 * 8}, {32 * 7, 32 * 8}, sprite_w);

		player.walking_frames[0] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 0, 32 * 1}, {32 * 3, 32 * 4}, sprite_w),
		player.walking_frames[1] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 1, 32 * 2}, {32 * 3, 32 * 4}, sprite_w);
		player.walking_frames[2] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 2, 32 * 3}, {32 * 3, 32 * 4}, sprite_w);
		player.walking_frames[3] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 3, 32 * 4}, {32 * 3, 32 * 4}, sprite_w);
		player.walking_frames[4] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 4, 32 * 5}, {32 * 3, 32 * 4}, sprite_w),
		player.walking_frames[5] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 5, 32 * 6}, {32 * 3, 32 * 4}, sprite_w);
		player.walking_frames[6] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 6, 32 * 7}, {32 * 3, 32 * 4}, sprite_w);
		player.walking_frames[7] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 7, 32 * 8}, {32 * 3, 32 * 4}, sprite_w);

		player.jump_frames[0] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 0, 32 * 1}, {32 * 2, 32 * 3}, sprite_w),
		player.jump_frames[1] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 1, 32 * 2}, {32 * 2, 32 * 3}, sprite_w);
		player.jump_frames[2] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 2, 32 * 3}, {32 * 2, 32 * 3}, sprite_w);
		player.jump_frames[3] = exctract_sprite_sheet_sample(sprite_sheet,  {32 * 3, 32 * 4}, {32 * 2, 32 * 3}, sprite_w);
	}

	player.startLevelTimer = SDL_GetTicks();

	float deltaTime = 0;
	float tile_size = WALL_SPRITE_X;

	window_canvas_t canvas_test = window_quad("shaders/renderer/pixel/fragment.glsl",
		"shaders/renderer/pixel/vertex.glsl", player.idle_frames[0], 32, 32, {0, SCREEN_WIDTH}, {0, SCREEN_HEIGHT});
	window_canvas_t playerr = window_quad("shaders/renderer/pixel/fragment.glsl",
		"shaders/renderer/pixel/vertex.glsl", tiles_sprite, tiles_sprite_w, tiles_sprite_h,
		{(tile_size), (tile_size) * 2}, {(tile_size), (tile_size) * 2});

	window_canvas_t tile = window_quad("shaders/renderer/pixel/fragment.glsl",
		"shaders/renderer/pixel/vertex.glsl", tiles_sprite, tiles_sprite_w, tiles_sprite_h,
		{(tile_size), (tile_size) * 2}, {(tile_size), (tile_size) * 2});
	{
		tile.scale = 1;
		tile.mousex = 0;
		tile.mousey = 0;
		tile.texturee.texture_data = map->wall_texture.texture_data;
		tile.texturee.texture_width = 16;
		tile.texturee.texture_height = 16;	
		tile.uv_side = {1, 1};
		tile.opacity = 1.0;
	}
		
	texture_t blood_tex;
	int ind = 0;
	const int max_splash = 500;
	vec2_t bloods[max_splash];

	blood_tex.texture_data = stbi_load("assets/blood.png", 
										&blood_tex.texture_width,
										&blood_tex.texture_height, 
										&blood_tex.channels,
										 4);

	window_canvas_t blood = window_quad("shaders/renderer/particle_system/fragment.glsl",
			"shaders/renderer/particle_system/vertex.glsl", tiles_sprite, tiles_sprite_w, tiles_sprite_h,
			{(tile_size), (tile_size) * 2}, {(tile_size), (tile_size) * 2});

	{
		blood.texturee.texture_data = blood_tex.texture_data;
		blood.texturee.texture_width = blood_tex.texture_width;
		blood.texturee.texture_height = blood_tex.texture_height;
		blood.scale = 4;
		blood.mousex = -player.x * (SCREEN_WIDTH/2);
		blood.mousey = player.y * (SCREEN_HEIGHT/2);
		blood.uv_side = {1, -1};
		blood.position = {0.0, 0.0};
		blood.opacity = 0.2;
		glGenBuffers(1, &blood.instance_buffer);
	}

	{
		playerr.texturee.texture_data = player.idle_frames[0];
		playerr.texturee.texture_width = 32;
		playerr.texturee.texture_height = 32;
		playerr.mousex = 0;
		playerr.mousey = 0;
		playerr.uv_side = {1, 1};
		playerr.scale = 1;
		playerr.position.x = player.x;
		playerr.position.y = player.y;

	}

	particle_t 			particle;
	
	particle.quad = window_quad("shaders/renderer/particle_system/fragment.glsl",
		"shaders/renderer/particle_system/vertex.glsl", tiles_sprite, tiles_sprite_w, tiles_sprite_h,
		{100, 200}, {200, 300});
		
	particle.instanciated_particles_number = 100;
	particle.quad.texturee.texture_data = blood_tex.texture_data;
	particle.quad.texturee.texture_width = blood_tex.texture_width;
	particle.quad.texturee.texture_height = blood_tex.texture_height;
	particle.quad.scale = 0.2;
	particle.quad.uv_side = {1, -1};
	particle.quad.opacity = 0.6;
	particle.quad.position = {0.0f, 0.0f};

	particle.initiate_particle({player.x, player.y});

	glGenBuffers(1, &particle.quad.instance_buffer);

	particle.force = 1.0f;
	particle.size = 1.0f;

	float f = 0;

	vec2_t camera2d = {0, 0};

	glEnable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glStencilFunc(GL_EQUAL, 0, 0X00);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	while (1) {		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();		
        ImGui::NewFrame();

		ImGui::Begin("Hello, world!");
		ImGui::Text("Particles");
		// ImGui::SliderFloat("speed", &particle.emission_speed, 0.0, 1);
		// // ImGui::SliderInt("numbers", &particle.instanciated_particles_number, 1, 50);
		ImGui::SliderFloat("size", &particle.quad.scale, -10, 10);
		// ImGui::SliderFloat("position x", &particle.quad.position.x, 0.0f, 0.5f);
		// ImGui::SliderFloat("position y", &particle.quad.position.y, 0.0f, 0.5f);
		ImGui::End();
		ImGui::Render();

		float currentTime = SDL_GetTicks();
		int w, h;

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glStencilFunc(GL_ALWAYS, 1, 0XFF);

		SDL_GetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h);

		render_scene(map, tile, (tile_size) * 2 - (tile_size), (tile_size) * 2 - (tile_size), camera2d);
		glStencilFunc(GL_EQUAL, 1, 0XFF);
		glStencilMask(0x00);
		render_quad_screen(blood, true, ind);

		playerr.position.x = player.x;
		playerr.position.y = player.y;

		if (player.currentAnim == ANIM_IDLE)
			playerr.texturee.texture_data = player.idle_frames[player.currentFrame];
		if (player.currentAnim == ANIM_RUN)
			playerr.texturee.texture_data = player.walking_frames[player.currentFrame];
		if (player.currentAnim == ANIM_JUMP)
			playerr.texturee.texture_data = player.jump_frames[player.currentFrame];

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{			
            ImGui_ImplSDL2_ProcessEvent(&ev);
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
							player.vy = 10 * JUMP_STRENGTH/((float)SCREEN_HEIGHT/2);
							player.onGround = false;
						}						
						// camera2d.y -= 11;
						break;
					case SDL_SCANCODE_S:
						break ;
					case SDL_SCANCODE_D:
						// camera2d.x -= PLAYER_SPEED * deltaTime;				
						break;
					case SDL_SCANCODE_A:					
						// camera2d.x += PLAYER_SPEED * deltaTime;
						break;
					case SDL_SCANCODE_Z:					
						tile.scale += 0.1;					
						playerr.scale += 0.1;													
						blood.scale += 0.1;			
						break;
					case SDL_SCANCODE_T:
						bloods[ind++] = {player.x, player.y};
						glBindBuffer(GL_ARRAY_BUFFER, blood.instance_buffer);
						glBufferData(GL_ARRAY_BUFFER, sizeof(vec2_t) * ind, &bloods[0], GL_STATIC_DRAW);
						glEnableVertexAttribArray(2);
						glBindBuffer(GL_ARRAY_BUFFER, blood.instance_buffer);
						glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2_t), (void*)0);
						glVertexAttribDivisor(2, 1);
						break;
					case SDL_SCANCODE_X:
						tile.scale -= 0.1;										
						playerr.scale -= 0.1;
						break;

					default:
						break;
				}
			}
		}

		glStencilMask(0x00);
        glStencilFunc(GL_ALWAYS, 0, 0x00);
		render_quad_screen(playerr);

		particle.update_particle(playerr.position, deltaTime);
		render_quad_screen(particle.quad, true, particle.instanciated_particles_number);

		glBindBuffer(GL_ARRAY_BUFFER, particle.quad.instance_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2_t) * particle.instanciated_particles_number, &particle.offset_intsances[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particle.quad.instance_buffer);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2_t), (void*)0);
		glVertexAttribDivisor(2, 1);

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);

		//
		{
		// --- Horizontal Movement ---
		float nextX = player.x; 
		bool isMoving = false;
		
		// // Only allow movement if not dying
		if (!player.isDying) {
			if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {
				nextX -= PLAYER_SPEED/(SCREEN_WIDTH/2) * deltaTime;
				playerr.uv_side.x = -1;
				player.facingRight = false;
				isMoving = true;
			}
			if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
				nextX += PLAYER_SPEED/(SCREEN_WIDTH/2) * deltaTime;
				player.facingRight = true;
				playerr.uv_side.x = 1;
				isMoving = true;
			}

			// Horizontal collision
			if (nextX > player.x) 
			{ // Moving right
				if (!checkWallCollision(nextX + player.width, player.y, map)) 
					player.x = nextX;
			}
			if (nextX < player.x) 
			{ // Moving left
				if (!checkWallCollision(nextX + 0.02, player.y, map))
					player.x = nextX;
			}
			player.vy -= GRAVITY/((float)SCREEN_HEIGHT/2);
			float nextY = player.y + player.vy * deltaTime;

			player.y = nextY;
			if (player.vy < 0) { // Moving down
				if (checkWallCollision(player.x, nextY, map) || checkWallCollision(player.x + player.width, nextY, map)) {
					// Snap to ground
					float tileHeight = WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);
					player.y = (int)((nextY)/tileHeight) * tileHeight + player.height;
					player.vy = 0;
					player.onGround = true;
				}
			} 
			else if (player.vy > 0) { // Moving up
				// if (checkWallCollision(player.x, nextY, map) || checkWallCollision(player.x + player.width - 1, nextY, map)) {
				// 	player.vy = 0;
				// }
			}
		}

		float tileWidth = (float)WALL_SPRITE_X;
		float tileHeight = (float)WALL_SPRITE_X;

		// --- Update Animation State ---
		if (!player.isDying && !player.isRespawning) {
			// Determine which animation to play based on player state
			AnimationType newAnim = ANIM_IDLE;
			
			if (!player.onGround) {
				// Player is in the air - jump animation
				newAnim = ANIM_JUMP;
			} else if (isMoving) {
				// Player is moving on ground - run animation
				newAnim = ANIM_RUN;
			} else {
				// Player is standing still - idle animation
				newAnim = ANIM_IDLE;
			}
			
			// If animation changed, reset frame
			if (newAnim != player.currentAnim) {
				player.currentAnim = newAnim;
				player.currentFrame = 0;
				player.lastFrameTime = SDL_GetTicks();
			}

			// Update animation frame based on time with accumulation
			Uint32 currentTime = SDL_GetTicks();
			Uint32 elapsedTime = currentTime - player.lastFrameTime;
			
			if (elapsedTime >= player.frameDelay) {
				// Calculate how many frames we should advance
				int framesToAdvance = elapsedTime / player.frameDelay;
				
				// // Update the last frame time, keeping the remainder for smooth timing
				player.lastFrameTime += framesToAdvance * player.frameDelay;
				
				// Get frame count for current animation
				int frameCount;
				switch (player.currentAnim) {
					case ANIM_IDLE: frameCount = 4; break;
					case ANIM_IDLE_BLINK: frameCount = 4; break;
					case ANIM_WALK: frameCount = 6; break;
					case ANIM_RUN: frameCount = 8; break;
					case ANIM_DUCK: frameCount = 4; break;
					case ANIM_JUMP: frameCount = 4; break;
					case ANIM_DISAPPEAR: frameCount = 6; break;
					case ANIM_DIE: frameCount = 8; break;
					case ANIM_ATTACK: frameCount = 6; break;
					default: frameCount = 4; break;
				}
				
				// Loop animation
				player.currentFrame = (player.currentFrame + framesToAdvance) % frameCount;
			}
		}
		}

		deltaTime = (SDL_GetTicks() - currentTime)/1000;
		SDL_SetWindowFullscreen(window, win_mode);		
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
