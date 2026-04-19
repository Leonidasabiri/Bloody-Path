#version 330 core

in vec2 uvtex;

out vec4 FragColor;

uniform float opacity;
uniform vec2 mouse;
uniform vec2 player_position;
uniform vec2 resolution;
uniform float time;
uniform vec2 uv;
uniform vec2 uv_side;
uniform sampler2D t;


void main()
{
    float darken = 1;

    vec2 uv = gl_FragCoord.xy/resolution;
    vec2 mos = mouse;
    vec2 playerpos = player_position;

    playerpos.x *= resolution.x/resolution.y;

    vec4 col = texture(t, uvtex * uv_side);

    col.w *= opacity;

    if (col.a == 0.0)
        discard;

    // if (uvtex.x <= 0.02 || uvtex.y <= 0.02 || uvtex.x >= 0.99|| uvtex.y >= 0.99) 
    //     col = vec4(0., 1., 0., 1.);

    FragColor =  vec4(col.xyzw);
}
