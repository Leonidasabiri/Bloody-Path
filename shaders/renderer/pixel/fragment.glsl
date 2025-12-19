#version 330 core

out vec4 FragColor;
in vec2 uvtex;

uniform sampler2D t;

void main()
{
   FragColor = texture(t, uvtex);
}