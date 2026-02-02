#version 330 core

uniform sampler2D render_pass;
uniform vec2 resolution;
uniform vec2 subject_position;
uniform float time;
uniform float opacity;
in vec2 uvtex;

out vec4 FragColor;

void main()
{
    float offset = 0.5;
    vec2 uv = gl_FragCoord.xy/resolution;
    vec4 blurred = (texture(render_pass, uv)/32); //
    vec4 tex = texture(render_pass, uv);

    // for (int i = 1; i < 5; i++)
    // {
    //     if (dot(tex, vec4(1., 1., 1., 1.)) < 0.5) continue;
    //     offset /= i;        
    //     blurred += texture(render_pass, uv + vec2(offset, 0.0)/22.)
    //              + texture(render_pass, uv + vec2(offset, offset)/22.)
    //              + texture(render_pass, uv + vec2(0.0, offset)/22.)
    //              + texture(render_pass, uv + vec2(-offset, offset)/22.)
    //              + texture(render_pass, uv + vec2(-offset, -offset)/22.)
    //              + texture(render_pass, uv + vec2(offset, -offset)/22.)
    //              + texture(render_pass, uv + vec2(0, -offset)/22.)/(15 * i);
    // }

    // if (blurred.x >= 0 && blurred.y >= 0)
    //      tex += blurred/15;
    
    FragColor = vec4(0., 0., 0., opacity);
}
