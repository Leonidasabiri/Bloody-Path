#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 uv;

out vec2 uvtex;
out vec3 p;

uniform vec2 player_position;
uniform vec2 mouse_position;
uniform float scale;

vec2 offset = vec2(-1.0, -1.0);

void main()
{
   uvtex = uv;
   p = aPos;

   p.xy += player_position;
   p.xy -= vec2(mouse_position.x, -mouse_position.y);
   p.x *= scale;
   p.y *= scale;
   p.xy += vec2(mouse_position.x, -mouse_position.y);

   gl_Position = vec4(p.x, p.y, p.z, 1.0);
}
