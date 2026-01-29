#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec2 offset;

out vec2 uvtex;
out vec3 p;

uniform vec2 player_position;
uniform vec2 mouse_position;
uniform float rotation_degree;
uniform float scale;
uniform bool instanced;

void main()
{
   float vertex_scale = scale;
   uvtex = uv;
   p = aPos;

   p.x *= vertex_scale + gl_InstanceID/100;
   p.y *= vertex_scale + gl_InstanceID/100;

   p.xy += player_position + offset;


   gl_Position = vec4(p.x, p.y, p.z, 1.0);
}
