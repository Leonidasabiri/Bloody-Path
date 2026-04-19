
#include "player.h"
#include "../config.h"

void findPlayerStart(Map* map, Player* player)
{
    if (!map || !player) return;
    float tileWidth = WALL_SPRITE_X;
    float tileHeight = WALL_SPRITE_X;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
            if (map->data[y][x] == 'P') {
                player->x = x * (WALL_SPRITE_X/((float)SCREEN_WIDTH/2));
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

Player setup_player(Player player, unsigned char* sprite_sheet, int sprite_w)
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
	player.touchedSpike = false;
	player.isMoving = false;
	player.spikeTimer = 0;
	player.deathAnimStartTime = 0;
	player.deathStartX = 0;
	player.deathStartY = 0;
	player.max_health = 30;
	player.health = player.max_health;
	player.travelStartTime = 0;

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
	
	player.startLevelTimer = SDL_GetTicks();

	return player;
}

bool checkExitCollisionLocal(Player* player, Map* map)
{
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

bool checkWallCollision(float x, float y, Map* map) 
{
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

bool checkSpikeCollision(Player* player, Map* map) 
{
    if (!map) return false;

    // Get player center
    float tileWidth = (float)WALL_SPRITE_X/((float)SCREEN_WIDTH/2);
    float tileHeight = (float)WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);

    int mapX = (int)(player->x/tileWidth);
    int mapY = map->height - (int)(player->y/tileHeight);


    if (mapX < 0 || mapX >= map->width || mapY < 0 || mapY >= map->height) {
        return false;
    }

    return map->tile_types[mapY][mapX] == TILE_SPIKE;
}

Player game_logic(Player player, window_canvas_t *playerr, Map *map, float *deltaTime,
			 	float *currentTime, const Uint8* keystates)
{
	// --- Horizontal Movement ---
	float nextX = player.x; 

	*deltaTime = (SDL_GetTicks() - *currentTime)/1000;
	*currentTime = SDL_GetTicks();

	// // Only allow movement if not dying
	if (!player.isDying && state == PLAYING)
	{
		if (keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A]) {	
			nextX -= PLAYER_SPEED/(SCREEN_WIDTH/2) * *deltaTime;
			playerr->uv_side.x = -1;
			player.facingRight = false;
			player.isMoving = true;
		}
		if (keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D]) {
			nextX += PLAYER_SPEED/(SCREEN_WIDTH/2) * *deltaTime;
			player.facingRight = true;
			playerr->uv_side.x = 1;
			player.isMoving = true;
		}

		// Horizontal collision
		if (nextX > player.x) 
		{ // Moving right
			if (!checkWallCollision(nextX + player.width, player.y, map)) 
				player.x = nextX;
		}
		if (nextX < player.x) 
		{ // Moving left
			if (!checkWallCollision(nextX + 0.02f, player.y, map))
				player.x = nextX;
		}
		player.vy -= GRAVITY/((float)SCREEN_HEIGHT/2);
		float nextY = player.y + player.vy * *deltaTime;

		player.y = nextY;
		if (player.vy < 0) 
		{ // Moving down
			if (checkWallCollision(player.x + 0.02f, nextY, map)
			|| checkWallCollision(player.x + player.width, nextY, map)) 
			{
				// Snap to ground
				float tileHeight = WALL_SPRITE_X/((float)SCREEN_HEIGHT/2);
				player.y = (int)((nextY)/tileHeight) * tileHeight + player.height;
				player.vy = 0;
				player.onGround = true;
			}
		}
	}

	checkCheckpointCollision(map, player.x, player.y, &player.checkPointx, &player.checkPointy);

	if (checkSpikeCollision(&player, map))
	{
		player.isDying = true;
		player.x = player.checkPointx;
		player.y = player.checkPointy;
	}
	if (player.isDying)
	{
		player.damaged = 12.0f;
		player.isDying = false;
	}

	float tileWidth = (float)WALL_SPRITE_X;
	float tileHeight = (float)WALL_SPRITE_X;

	// --- Update Animation State ---
	if (!player.isDying && !player.isRespawning) {
		AnimationType newAnim = ANIM_IDLE;
		// Determine which animation to play based on player state
		if (!player.onGround) {
			// Player is in the air - jump animation
			newAnim = ANIM_JUMP;
		} else if (player.isMoving) {
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
	return player;
}


window_canvas_t animation(window_canvas_t playerr, Player player)
{
    switch (player.currentAnim)
    {
        case ANIM_IDLE:
            playerr.texturee.texture_data = player.idle_frames[player.currentFrame];
            break;
        case ANIM_RUN:
            playerr.texturee.texture_data = player.walking_frames[player.currentFrame];
            break;
        case ANIM_JUMP:
            playerr.texturee.texture_data = player.jump_frames[player.currentFrame];
            break;
        default:
            break;
    }

    return playerr;
}