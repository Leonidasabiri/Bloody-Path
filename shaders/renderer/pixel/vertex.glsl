#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 uv;

out vec2 uvtex;
out vec3 p;

uniform vec2 player_position;
uniform float scale;

void main()
{
   uvtex = uv;
   p = aPos;

   p.xy += player_position;
   p.x *= scale;
   p.y *= scale;

   gl_Position = vec4(p.x, p.y, p.z, 1.0);
}
