#ifndef PLAYER_H
#define PLAYER_H

#include "../engine/state/game_state.h"
#include "../engine/tools/sprite_sampler.h"
#include "../engine/tools/map_parser.h"

typedef enum {
    ANIM_IDLE = 0,
    ANIM_IDLE_BLINK = 1,
    ANIM_WALK = 2,
    ANIM_RUN = 3,
    ANIM_DUCK = 4,
    ANIM_JUMP = 5,
    ANIM_DISAPPEAR = 6,
    ANIM_DIE = 7,
    ANIM_ATTACK = 8
} AnimationType;

typedef struct {
    float x, y;
    float width, height;
    float vy; // Vertical velocity
    float vx; // Vertical velocity
    bool onGround;
    bool touchedSpike; // Flag to track if player has touched a spike
    float checkpointX; // X coordinate of last checkpoint
    float checkpointY; // Y coordinate of last checkpoint
    Uint32 spikeTimer; // Timer for spike respawn
    Uint32 startLevelTimer;

    float health;
    float max_health;

    // Animation fields
    AnimationType currentAnim;
    int currentFrame;
    Uint32 lastFrameTime;
    Uint32 frameDelay; // Milliseconds per frame (changed to Uint32 to match SDL_GetTicks)
    bool facingRight; // Direction player is facing
    bool isDying; // Flag for death animation
    bool isWaitingToRespawn; // Flag to wait for death animation to finish
    bool isRespawning; // Flag for respawn animation (death played backwards)
    bool isMoving;
    Uint32 deathAnimStartTime; // When death animation started
    float deathStartX; // Position when death animation started
    float deathStartY; // Position when death animation started
    float checkPointx;
    float checkPointy;
    float damaged;
    Uint32 travelStartTime; // When the last frame starts traveling to respawn
    GLuint         textures_ids[48];    // for all the frames of all the animations
    unsigned char *death_frames[8];
    unsigned char *walking_frames[8];
    unsigned char *idle_frames[4];
    unsigned char *jump_frames[4];
} Player;

void renderPlayer(unsigned char* pixels, Player* player, unsigned char*);
Player setup_player(Player player, unsigned char* sprite_sheet, int sprite_w);
Player game_logic(Player player, window_canvas_t *playerr, Map *map, float *deltaTime,
			 	float *currentTime, const Uint8* keystates);
void findPlayerStart(Map* map, Player* player);
bool checkWallCollision(float x, float y, Map* map);
bool checkExitCollision(Map* map, int playerX, int playerY);
bool checkSpikeCollision(Player* player, Map* map);
bool checkExitCollisionLocal(Player* player, Map* map);
window_canvas_t animation(window_canvas_t playerr, Player player);

#endif // PLAYER_H
