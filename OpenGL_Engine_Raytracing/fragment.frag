#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec3 objectColor;
uniform float roughness;
uniform float metallic;

uniform vec3 cameraPosition;
uniform vec3 lightPos;
uniform vec3 lightColor;

out vec4 fragColor;

void main()
{
    fragColor = vec4(lightColor * objectColor, 1.f);
}
