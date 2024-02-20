#version 330 core

uniform vec3 albedoColor;

out vec4 fragColor;

void main()
{
    fragColor = vec4(albedoColor, 1);
}
