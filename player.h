#ifndef PLAYER_H
#define PLAYER_H

typedef struct {
    float x, y;
    float width, height;
    float vy; // Vertical velocity
    bool onGround;
    bool touchedSpike; // Flag to track if player has touched a spike
    Uint32 spikeTimer; // Counter in milliseconds since touching spike
} Player;

#endif // PLAYER_H
