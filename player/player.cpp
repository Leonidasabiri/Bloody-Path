

#include <SDL.h>
#include <SDL_opengl.h>
#include "player.h"

Player setup_player(Uint32 frame_delay, bool facing_right, float max_health)
{
    Player player;

    player.currentAnim = ANIM_IDLE;
    player.currentFrame = 0;
    player.lastFrameTime = SDL_GetTicks();
    player.frameDelay = 100; // Changed from 150ms to 200ms for slower animation
    player.facingRight = false;
    player.isDying = false;
    player.isWaitingToRespawn = false;
    player.isRespawning = false;
    player.deathAnimStartTime = 0;
    player.deathStartX = 0;
    player.deathStartY = 0;
    player.max_health = max_health;
    player.health = player.max_health;
    player.travelStartTime = 0;    
    player.startLevelTimer = SDL_GetTicks();

    return player;
}
