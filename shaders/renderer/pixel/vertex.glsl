#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec2 of;

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

   p.xy += player_position + vec2(mouse_position.x, -mouse_position.y) + 1 + of;
   p.x *= scale;
   p.y *= scale;

   gl_Position = vec4(p.x, p.y, p.z, 1.0);
}
