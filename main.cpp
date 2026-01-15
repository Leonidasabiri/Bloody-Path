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
#include "engine/tools/map_parser.h"
#include "engine/rendering/renderer.h"
#include "player/player.h"

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
                player->width = WALL_SPRITE_X;
                player->height = WALL_SPRITE_X;
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

    float tileWidth = (float)WALL_SPRITE_X;
    float tileHeight = (float)WALL_SPRITE_X;

    int mapX = (int)(x /((float)SCREEN_WIDTH/2));
    int mapY = (int)(y /((float)SCREEN_HEIGHT/2));

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

void render_scene(Map *map, window_canvas_t quad, float w, float h, vec2_t offset)
{
	for (int y = 0; y < map->height + 0 ; ++y)
	{
		for (int x = 0; x < map->width + 0 ; ++x)
		{
			switch (map->tile_types[y][x])
			{
				case TILE_WALL:
					// quad.texturee.texture_data   = map->wall_texture.texture_data;
					// quad.texturee.texture_width  = map->wall_texture.texture_width;
					// quad.texturee.texture_height = map->wall_texture.texture_height;
					break;
				case TILE_SPIKE:
					// quad.texturee.texture_data   = map->spike_texture.texture_data;
					// quad.texturee.texture_width  = map->spike_texture.texture_width;
					// quad.texturee.texture_height = map->spike_texture.texture_height;
				default:
					break;
			}
			if (map->tile_types[y][x] != TILE_EMPTY && map->tile_types[y][x] != TILE_PLAYER_START)
			{
				quad.position.x = w/((float)SCREEN_WIDTH/2) * x + offset.x/((float)SCREEN_WIDTH/2);
				quad.position.y = h/((float)SCREEN_HEIGHT/2) * (map->height - y) + offset.y/((float)SCREEN_HEIGHT/2);
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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
	map->wall_texture.texture_data = stbi_load("assets/brick.jpg", &map->wall_texture.texture_width,
																	&map->wall_texture.texture_height, 
																	&map->wall_texture.channels, 4);

	Player player;
	findPlayerStart(map, &player);
	// the whole sprite sheet goes here
	unsigned char* sprite_sheet = stbi_load("assets/Hooded Protagonist Animation Sheet.png", &sprite_w, &sprite_h, &channels, 4);
	unsigned char* tiles_sprite = stbi_load("assets/brick.jpg", &tiles_sprite_w, &tiles_sprite_h, &tiles_sprite_channels, 4);
	(void)tiles_sprite;  // Suppress unused variable warning

	player.touchedSpike = false;
	player.spikeTimer = 0;

	float time = 0;

	const Uint8* keystates = SDL_GetKeyboardState(NULL);
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
	playerr.scale = 1;
	window_canvas_t tile = window_quad("shaders/renderer/pixel/fragment.glsl",
		"shaders/renderer/pixel/vertex.glsl", tiles_sprite, tiles_sprite_w, tiles_sprite_h,
		{(tile_size), (tile_size) * 2}, {(tile_size), (tile_size) * 2});
	tile.scale = 1;
	tile.mousex = 0;
	tile.mousey = 0;

	tile.texturee.texture_data = tiles_sprite;
	tile.texturee.texture_width = tiles_sprite_w;
	tile.texturee.texture_height = tiles_sprite_h;
	
	playerr.texturee.texture_data = player.idle_frames[0];
	playerr.texturee.texture_width = 32;
	playerr.texturee.texture_height = 32;
	playerr.mousex = 0;
	playerr.mousey = 0;

	float f = 0;

	vec2_t camera2d = {0, 0};

	while (1) {
		
		float currentTime = SDL_GetTicks();
		int w, h;
		glEnable(GL_BLEND);        
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GetMouseState(&tile.mousex, &tile.mousey);
		SDL_GetMouseState(&playerr.mousex, &playerr.mousey);
		SDL_GetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h);

		render_scene(map, tile, (tile_size) * 2 - (tile_size), (tile_size) * 2 - (tile_size), camera2d);

		playerr.position.x = player.x;
		playerr.position.y = player.y;

		playerr.texturee.texture_data = player.idle_frames[(int)f%4];
		render_quad_screen(playerr);

		f += 0.1;

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
					case SDL_SCANCODE_UP:
					case SDL_SCANCODE_W:
						if (player.onGround) {
							player.vy = JUMP_STRENGTH;
							player.onGround = false;
						}						
						camera2d.y -= 11;
						break;
					case SDL_SCANCODE_S:
						camera2d.y += 11;
						break ;
					case SDL_SCANCODE_D:
						camera2d.x -= PLAYER_SPEED * deltaTime;
						break;
					case SDL_SCANCODE_A:					
						camera2d.x += PLAYER_SPEED * deltaTime;
						break;
					case SDL_SCANCODE_Z:					
						tile.scale += 0.1;					
						playerr.scale += 0.1;
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

		time += 0.01;

		Uint32 impact_seconds_elapsed = SDL_GetTicks() - player.startLevelTimer;
		float impact_seconds = impact_seconds_elapsed / 1000.0f;

		if (impact_seconds >= 10) 
		{
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
				player.health = player.max_health;
				player.startLevelTimer = SDL_GetTicks();
			}         
		}


		// 
		{
		// --- Horizontal Movement ---
		float nextX = player.x; 
		bool isMoving = false;
		
		// // Only allow movement if not dying
		if (!player.isDying) {
			if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {
				nextX -= PLAYER_SPEED * deltaTime;
				player.facingRight = false;
				isMoving = true;
			}
			if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
				nextX += PLAYER_SPEED * deltaTime;
				player.facingRight = true;
				isMoving = true;
			}

			// Horizontal collision
			if (nextX > player.x) { // Moving right
				if (!checkWallCollision(nextX + player.width, player.y, map) 
				&& !checkWallCollision(nextX + player.width, player.y + player.height - 1, map)) 
				{
					player.x = nextX;
				}
			} else if (nextX < player.x) { // Moving left
				if (!checkWallCollision(nextX, player.y, map) 
				&& !checkWallCollision(nextX, player.y + player.height - 1, map))
				{
					player.x = nextX;
				}
			}
			// player.vy += GRAVITY;
			float nextY = player.y + player.vy  * deltaTime;

			player.onGround = false; // Assume not on ground until proven otherwise

			if (player.vy > 0) { // Moving down
				if (checkWallCollision(player.x, nextY + player.height, map) || checkWallCollision(player.x + player.width - 1, nextY + player.height, map)) {
					// Snap to ground
					float tileHeight = WALL_SPRITE_X;
					// player.y = (int)((nextY + player.height) / tileHeight) * tileHeight - player.height;
					// player.vy = 0;
					// player.onGround = true;
				} 
				else 
				{
					// player.y = nextY;
				}
			} else if (player.vy < 0) { // Moving up
				if (checkWallCollision(player.x, nextY, map) || checkWallCollision(player.x + player.width - 1, nextY, map)) {
					// player.vy = 0;
				} else {
					// player.y = nextY;
				}
			}
		}

		float tileWidth = (float)WALL_SPRITE_X;
		float tileHeight = (float)WALL_SPRITE_X;

		if (checkSpikeCollision(&player, map)) {
			if (!player.touchedSpike) {
				// First time touching spike - start the timer
				player.touchedSpike = true;
				player.spikeTimer = SDL_GetTicks();
			}
		}

		Uint32 elapsedTime = SDL_GetTicks() - player.spikeTimer;
		// Check if 10 seconds have passed since touching spike
		if (player.touchedSpike && !player.isDying && !player.isWaitingToRespawn && !player.isRespawning) {
			Uint32 elapsedTime = SDL_GetTicks() - player.spikeTimer;
			float seconds = elapsedTime / 1000.0f;

			if (seconds >= 0.0f) {
				// 10 seconds have passed - start death animation
				player.isDying = true;
				player.isWaitingToRespawn = true;
				player.currentAnim = ANIM_DIE;
				player.currentFrame = 0;
				player.lastFrameTime = SDL_GetTicks();
				player.deathAnimStartTime = SDL_GetTicks();
				// Store starting position (death stays at this location)
				player.deathStartX = player.x;
				player.deathStartY = player.y;
				player.health -= 4.5;
			}
		}
		// Handle death animation, travel, and respawn sequence
		if (player.isWaitingToRespawn && !player.isRespawning) {
			Uint32 deathAnimElapsed = SDL_GetTicks() - player.deathAnimStartTime;
			// Death animation has 8 frames at 150ms each = 1200ms total
			const Uint32 DEATH_ANIM_DURATION = 8 * 150; // 8 frames * 150ms per frame
			
			// Stay at death position during death animation
			player.x = player.deathStartX;
			player.y = player.deathStartY;
			
			if (deathAnimElapsed >= DEATH_ANIM_DURATION) {
				// Death animation complete - start traveling last frame to checkpoint
				player.isWaitingToRespawn = false;
				player.travelStartTime = SDL_GetTicks();
				// Keep last frame of death animation (frame 7)
				player.currentFrame = 7;
			}
		}
		// Travel the last death frame to respawn location
		if (!player.isWaitingToRespawn && !player.isRespawning && player.isDying && player.travelStartTime > 0) {
			Uint32 travelElapsed = SDL_GetTicks() - player.travelStartTime;
			const Uint32 TRAVEL_DURATION = 800; // 800ms to travel to checkpoint
			
			// Smoothly interpolate position to respawn point
			float progress = (float)travelElapsed / (float)TRAVEL_DURATION;
			if (progress > 1.0f) progress = 1.0f;
			
			// Calculate target respawn position
			// Position player so feet are at the bottom of the checkpoint tile
			float targetX = player.checkpointX * tileWidth;
			float targetY = (player.checkpointY + 1) * tileHeight - player.height;
			
			// Interpolate position (ease-in-out for smoother motion)
			float easedProgress = progress * progress * (3.0f - 2.0f * progress); // Smoothstep
			player.x = player.deathStartX + (targetX - player.deathStartX) * easedProgress;
			player.y = player.deathStartY + (targetY - player.deathStartY) * easedProgress;
			
			// Keep showing last frame of death animation during travel
			player.currentFrame = 7;
			
			if (travelElapsed >= TRAVEL_DURATION) {
				// Travel complete - start respawn animation (play death backwards)
				player.isRespawning = true;
				player.currentFrame = 7; // Start from last frame
				player.lastFrameTime = SDL_GetTicks();
				player.travelStartTime = 0;
				
				// Ensure we're exactly at checkpoint position
				player.x = targetX;
				player.y = targetY;
			}
		}

		// Play respawn animation (death animation backwards)
		if (player.isRespawning) {
			Uint32 currentTime = SDL_GetTicks();
			Uint32 elapsedTime = currentTime - player.lastFrameTime;
			
			if (elapsedTime >= player.frameDelay) {
				player.lastFrameTime = currentTime;
				
				// Go backwards through death animation frames
				player.currentFrame--;
				
				if (player.currentFrame < 0) {
					// Respawn animation complete - return to normal gameplay
					
					// Reset all death and respawn flags
					player.touchedSpike = false;
					player.spikeTimer = 0;
					player.isDying = false;
					player.isRespawning = false;
					player.deathAnimStartTime = 0;
					player.deathStartX = 0;
					player.deathStartY = 0;
					
					// Return to idle animation
					player.currentAnim = ANIM_IDLE;
					player.currentFrame = 0;
					player.lastFrameTime = SDL_GetTicks();
					player.vy = 0;
					player.onGround = false;
				}
			}
		}

		// Check for checkpoint collision and save respawn point
		float playerCenterX = player.x + player.width / 2;
		float playerCenterY = player.y + player.height / 2;
		int gridX = (int)(playerCenterX / tileWidth);
		int gridY = (int)(playerCenterY / tileHeight);
		if (::checkCheckpointCollision(map, gridX, gridY, &player.checkpointX, &player.checkpointY)) {
			player.health = player.max_health;
			player.startLevelTimer = SDL_GetTicks();
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
					player.health = player.max_health;
					player.startLevelTimer = SDL_GetTicks();
				}         
			}
			else {
			// Debug: Check if player is near the exit
				float playerCenterX_exit = player.x + player.width / 2;
				float playerBottomY = player.y + player.height;
				int exitGridX = (int)(playerCenterX_exit / tileWidth);
				int exitGridY = (int)(playerBottomY / tileHeight);
				
				// Only print when player is near the exit area
				if (exitGridX >= 18 && exitGridY >= 10) {
					printf("Player pos: (%.1f, %.1f) size: (%.1f, %.1f) Grid: (%d, %d) Exit at: (%d, %d)\n", 
						player.x, player.y, player.width, player.height, 
						exitGridX, exitGridY, map->exit.x, map->exit.y);
			}
		}

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
		if (player.isDying && !player.isRespawning && player.isWaitingToRespawn) {
			Uint32 currentTime = SDL_GetTicks();
			Uint32 elapsedTime = currentTime - player.lastFrameTime;
			
			if (elapsedTime >= player.frameDelay && player.currentFrame < 7) {
				int framesToAdvance = elapsedTime / player.frameDelay;
				player.lastFrameTime += framesToAdvance * player.frameDelay;
				
				// Advance death animation, stop at last frame
				player.currentFrame += framesToAdvance;
				if (player.currentFrame > 7) { // Ensure we don't go past frame 7
					player.currentFrame = 7;
				}
			}
		}
		}
		
		deltaTime = (SDL_GetTicks() - currentTime)/1000;
		SDL_SetWindowFullscreen(window, win_mode);
		SDL_GL_SwapWindow(window);

	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
