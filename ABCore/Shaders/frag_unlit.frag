#version 330 core

uniform vec3 albedoColor;
uniform bool topHalf;
uniform int size;

out vec4 fragColor;

void main()
{
    fragColor = vec4(topHalf && gl_FragCoord.y < size / 2.f ? vec3(0) : albedoColor, 1);
}
