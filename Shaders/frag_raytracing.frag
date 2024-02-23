#version 450 core

layout (binding = 0) uniform samplerBuffer triData;

uniform vec3 albedoColor;

out vec4 fragColor;

void main()
{
    fragColor = vec4(albedoColor, 1);
}
