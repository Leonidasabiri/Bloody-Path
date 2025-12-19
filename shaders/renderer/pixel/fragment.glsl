#version 330 core

in vec2 uvtex;

uniform vec2 mouse;
uniform vec2 resolution;

uniform sampler2D t;

void main()
{
    float darken = 4;

    vec2 uv = gl_FragCoord.xy/resolution * 2. - 1.;

    float light = 1/(length(uv - mouse) * 2.);
    vec4 lightColor = vec4(vec3(light) * 0.1, 1.0);

    vec4 text = texture(t, uvtex)/darken;

    gl_FragColor = lightColor;
}
