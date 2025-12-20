#ifndef CONFIG_H
#define CONFIG_H

// Map is 24 wide x 13 tall
// To make square tiles: if each tile is 40x40 pixels
// Screen would be: 24 * 40 = 960 width, 13 * 40 = 520 height
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 520
#define PLAYER_SPEED 5.0f
#define GRAVITY 0.5f
#define JUMP_STRENGTH -12.0f

#endif // CONFIG_H

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
