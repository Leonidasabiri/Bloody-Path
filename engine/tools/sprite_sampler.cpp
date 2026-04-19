
#include "sprite_sampler.h"

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

