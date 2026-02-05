
#version 330 core

uniform vec2 resolution;
uniform vec2 player_position;
uniform vec2 player_velocity;
uniform float time;
uniform float opacity;
uniform float space_size;
in vec2 uvtex;

out vec4 FragColor;

float rand(vec2 uv)
{    
    return fract(sin(dot(uv.xy, vec2(12.98,78.233)))* 43758.5453123);
}

float noise(vec2 uv)
{
    vec2 i = floor(uv);
    vec2 f = fract(uv);

    f = f * f * (3. - 2. * f);

    float ux1 = rand(i);
    float ux2 = rand(i + vec2(1.0, 0.0));
    float ux3 = rand(i + vec2(0.0, 1.0));
    float ux4 = rand(i + vec2(1.0, 1.0));

    return mix(mix(ux1, ux2, f.x), mix(ux3, ux4, f.x), f.y);
}


void main()
{
    float offset = 0.5;
    vec2 uv = gl_FragCoord.xy/resolution * 2. - 1.;

    uv.x *= resolution.x/resolution.y;

    float distance = length(uv - player_position);
    float fog_alter = 10;

    vec2 v = uv;
    uv *= 2;
    uv.x = (uv.x - time/2);
    uv.y = (uv.y - time/2);

    distance += noise(uv)/10;
    distance += noise(uv*2)/10;
    distance += noise(uv*4)/10;
    distance *= space_size/1.4;
 
    distance += dot(player_velocity,v)/11.;

    vec4 color = vec4(vec4(0., 0., 0., opacity));

    color = vec4(smoothstep(0.5, 0.2, distance), 0.0, 0.0, smoothstep(0.2, 0.5, distance) - 0.02);

    FragColor = color;
}
