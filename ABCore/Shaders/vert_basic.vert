#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

uniform mat4 world;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * world * vec4(vPos, 1.0);
}
