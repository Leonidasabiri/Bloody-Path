#ifndef TINY_UTILS
#define TINY_UTILS

#include <iostream>

struct vec2_t
{
    float x, y;
    float dot(vec2_t vector2);    
    vec2_t operator + (vec2_t&);  
    vec2_t operator += (vec2_t&);
    vec2_t operator - (vec2_t&);
    vec2_t operator * (float);
    vec2_t normalize_vector();
    vec2_t scale(float);
};

// void draw_pixel(int x, int y, color_t color, GameRenderer game_renderer);


// vec2_t normalize_vector(vec2_t vector)
// {
//     // float length = sqrt(dot_product(vector, vector));

//     return { vector.x / length, vector.y / length};
// }

#endif