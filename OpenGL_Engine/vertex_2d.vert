#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;

out vec3 fColor;

void main()
{
    gl_Position = vPos;
    fColor = vColor;
}
