#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdbool.h>
#include "tinyutils.h"
#include "config.h"
#include "engine/state/game_state.h"
#include "engine/gui/imgui.h"
#include "engine/gui/imgui_impl_sdl2.h"
#include "engine/gui/imgui_impl_opengl3.h"
#include "engine/tools/map_parser.h"
#include "engine/rendering/renderer.h"
#include "engine/tools/map_renderer.h"
#include "player/player.h"
#include <time.h>

const char *game_name = "Bloody Path";

game_state_t state = MAIN_MENU;

float lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

int main(int argc, char* argv[]) 
{
	(void)argc;  // Suppress unused parameter warning
	(void)argv;  // Suppress unused parameter warning

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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

	texture_t game_menu_texture;

	game_menu_texture.texture_data = stbi_load("assets/title_card.png", &game_menu_texture.texture_width, &game_menu_texture.texture_height, &channels, 4);
	window_canvas_t game_menu_title = window_quad("shaders/renderer/pixel/fragment.glsl",
		"shaders/renderer/pixel/vertex.glsl", game_menu_texture.texture_data, 32, 132, {0, SCREEN_WIDTH}, {0, SCREEN_HEIGHT});

	game_menu_title.uv_side = {1, 1};
	game_menu_title.position = {-1.0, 0.8};
	game_menu_title.global_scale = 0.5;
	game_menu_title.texturee.texture_width = game_menu_texture.texture_width;
	game_menu_title.texturee.texture_height = game_menu_texture.texture_height;

	texture_t light_map_debug_texture_2d;

	game_menu_texture.texture_data = stbi_load("assets/title_card.png", &game_menu_texture.texture_width, &game_menu_texture.texture_height, &channels, 4);

	Player player;
	findPlayerStart(map, &player);
	// the whole sprite sheet goes here
	unsigned char* sprite_sheet = stbi_load("assets/Hooded Protagonist Animation Sheet.png", &sprite_w, &sprite_h, &channels, 4);
	unsigned char* tiles_sprite = stbi_load("assets/Dungeon_Tileset.png", &tiles_sprite_w, &tiles_sprite_h, &tiles_sprite_channels, 4);

	populate_map_texture(map, tiles_sprite, tiles_sprite_w);

	const Uint8* keystates = SDL_GetKeyboardState(NULL);

	player = setup_player(player, sprite_sheet, sprite_w);

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
		tile.global_scale = 3;
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
		blood.scale = 5.0f;
		blood.global_scale = 3.0f;
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
		playerr.global_scale = 3;
		playerr.position.x = player.x;
		playerr.position.y = player.y;
	}

	float f = 0;

	vec2_t camera2d = {0.0f, 0.0f};

	glEnable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glStencilFunc(GL_EQUAL, 0, 0X00);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	
	vec2_t tmpblood[max_splash];
	vec2_t tmppix[100];
	float currentTime = SDL_GetTicks();

	window_canvas_t screen_space = window_quad("shaders/post_processing/shadow_fog.glsl",
			"shaders/renderer/pixel/vertex.glsl", tiles_sprite, tiles_sprite_w, tiles_sprite_h,
			{0, SCREEN_WIDTH}, {0, SCREEN_HEIGHT});

	screen_space.position = {0,0};
	screen_space.opacity = 0.9f;	

	float checkPointx = player.x;
	float checkPointy = player.y;

	while (1) 
	{
		vec2_t tmp = camera2d;
		camera2d = {-player.x + (float)rand()/(float)RAND_MAX * player.damaged/200, 
					-player.y + (float)rand()/(float)RAND_MAX * player.damaged/200};

		playerr.position.x = player.x + camera2d.x;//
		playerr.position.y = player.y + camera2d.y;//

		if (player.damaged > 0)
			player.damaged -= 0.2f;
		else
			player.damaged = 0.0f;

		static float fog_radius = 0.0f;
		
		int w, h;

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glStencilFunc(GL_ALWAYS, 1, 0XFF);

		SDL_GetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h);

		render_scene(map, tile, (tile_size) * 2 - (tile_size), (tile_size) * 2 - (tile_size), camera2d);				

		playerr = animation(playerr, player);

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
						SDL_GetWindowSize(window, &w, &h);
						printf("%d %d\n", w, h);
						break;						
					case SDL_SCANCODE_SPACE:
						state = TRANSITION_TO_PLAYING;
						break;
					case SDL_SCANCODE_ESCAPE:
						return 0;  
					case SDL_SCANCODE_UP:
					case SDL_SCANCODE_W:
						if (player.onGround) {
							player.vy = 10 * JUMP_STRENGTH/((float)SCREEN_HEIGHT/2);
							player.onGround = false;
						}
						break;
					case SDL_SCANCODE_O:
						blood.scale += 0.1f;				
						break;
					case SDL_SCANCODE_Z:					
						tile.global_scale += 0.01f;
						playerr.global_scale += 0.01f;
						blood.global_scale += 0.01f;
						break;
					case SDL_SCANCODE_T:					
						player.damaged = 12.0f;
						bloods[ind++] = {playerr.position.x - camera2d.x, playerr.position.y - camera2d.y};		
						create_instance(blood, sizeof(vec2_t), bloods, ind);
						break;
					case SDL_SCANCODE_X:
						tile.global_scale -= 0.01f;
						playerr.global_scale -= 0.01f;
						blood.global_scale -= 0.01f;
						break;

					default:
						break;
				}
			}
		}

		if (state == TRANSITION_TO_PLAYING)
		{
			static float scale_step = 0.0f;
			tile.global_scale = lerp(tile.global_scale, 1.0f, scale_step);
			playerr.global_scale = lerp(playerr.global_scale, 1.0f, scale_step);
			blood.global_scale = lerp(blood.global_scale, 1.0f, scale_step);
			scale_step+=0.0001;
			if ((tile.global_scale) <= 1.0002 && 
				(playerr.global_scale) <= 1.0002 &&
				(blood.global_scale) <= 1.0002)
				state = PLAYING;
		}

		glStencilMask(0x00);
        glStencilFunc(GL_ALWAYS, 0, 0x00);
		setup_quad_screen(screen_space);
		shader_float_value(screen_space.shader_program, fog_radius, "space_size");
		shader_float2_value(screen_space.shader_program, (tmp.x - camera2d.x), -(tmp.y - camera2d.y), "player_velocity");
		render_quad_screen(screen_space, w, h);

		setup_quad_screen(playerr);
		render_quad_screen(playerr, w, h);

		if (state == MAIN_MENU)
		{
			setup_quad_screen(game_menu_title);
			render_quad_screen(game_menu_title, w, h);
		}

		glStencilFunc(GL_EQUAL, 1, 0XFF);
		glStencilMask(0x00);

		for (int i = 0 ; i < ind; i++)		
		{
			tmpblood[i] = bloods[i];
			tmpblood[i].x += camera2d.x;
			tmpblood[i].y += camera2d.y;
			glBindVertexArray(blood.vertex_array);
			glBindBuffer(GL_ARRAY_BUFFER, blood.instance_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec2_t) * ind, &tmpblood[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, blood.instance_buffer);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2_t), (void*)0);
			glVertexAttribDivisor(2, 1);
		}

		setup_quad_screen(blood);
		render_quad_screen(blood, 0, 0, true, ind);
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);

		player = game_logic(player, &playerr, map, &deltaTime, &currentTime, keystates);

		SDL_SetWindowFullscreen(window, win_mode);		
		SDL_GL_SwapWindow(window);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
