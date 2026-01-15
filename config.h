#ifndef CONFIG_H
#define CONFIG_H

#define SCREEN_WIDTH 840
#define SCREEN_HEIGHT 480
#define PLAYER_SPEED 122.0f
#define GRAVITY 2.0f
#define JUMP_STRENGTH -59.0f

#define WALL_SPRITE_X 30
#define WALL_SPRITE_Y 20

#endif // CONFIG_H

extern int mousex;
extern int mousey;

// Center Wall: White
// Top Edge: Red
// Bottom Edge: Lime Green
// Left Edge: Blue
// Right Edge: Yellow
// Top-Left Corner: Magenta
// Top-Right Corner: Cyan
// Bottom-Left Corner: Orange
// Bottom-Right Corner: Purple
// Inner Corners: A very light grey to distinguish them from the background.
// Indestructible Wall: A very dark grey.
