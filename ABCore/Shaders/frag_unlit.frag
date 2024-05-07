#version 450 core

uniform vec3 albedoColor;
uniform bool topHalf;
uniform int size;

layout (location = 0) out vec3 fragColor;

void main()
{
    fragColor = topHalf && gl_FragCoord.y < size / 2.f ? vec3(0) : albedoColor;
}
