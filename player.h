#ifndef PLAYER_H
#define PLAYER_H

typedef struct {
    float x, y;
    float width, height;
    float vy; // Vertical velocity
    bool onGround;
} Player;

#endif // PLAYER_H
