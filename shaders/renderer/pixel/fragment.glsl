#version 330 core

in vec2 uvtex;

uniform vec2 mouse;
uniform vec2 player_position;
uniform vec2 resolution;
uniform float time;

uniform sampler2D t;

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
    float darken = 1;

    vec2 uv = gl_FragCoord.xy/resolution * 2. - 1.;
    vec2 mos = mouse;
    vec2 playerpos = player_position;

    uv.x *= resolution.x/resolution.y;
    mos.x *= resolution.x/resolution.y;
    playerpos.x *= resolution.x/resolution.y;

    float light = (length((uv - playerpos))) + noise(uv - playerpos + time/12 + 10.2)/2 - 0.32;

    vec4 lightColor = vec4(vec3(light), 1.0);

    if (light < 0.35)
        lightColor = texture(t, uvtex) + (vec4(1./(1 - light)/3.) 
                    * mix(vec4(0.75, 0.0, 0., 1.), 
                          vec4(0.0, 0.0, 0., 1.), light) * vec4(1.4, 0., 0., 1.))/32.4;
    else
    {
        lightColor = texture(t, uvtex)/(darken * 15 * light) 
                    + vec4((light * 1.2)/3. + 0.2) 
                    * mix(vec4(1.0, 0.0, 0., 1.), 
                          vec4(0.0, 0.0, 0., 1.), light) * vec4(1.4, 0., 0., 1.);

    }

    if (texture(t, uvtex) == vec4(1. , 0., 0., 1.))
        lightColor = vec4(1. , 0., 0., 1.);
    gl_FragColor = lightColor;
}

