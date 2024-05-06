#version 330 core

uniform vec3 albedoColor;
uniform bool topHalf;
uniform int size;

out vec4 fragColor;

void main()
{
    vec2 coord = (gl_FragCoord.xy) / float(size) * 2.f - 1.f; // [-1, 1]
    vec3 color = (topHalf && coord.y <= 0.f) ? vec3(0) : albedoColor;

    // calculate delta form factor and pass it in as the color's w component
    float dA = (topHalf ? max(coord.y, 0.f) : 1.f) / float(size * size);
    float dF = dA / (3.141592654f * pow(1.f + pow(coord.x, 2) + pow(coord.y, 2), 2.f));

    fragColor = vec4(color, dF * color.x * 100.f);
}
