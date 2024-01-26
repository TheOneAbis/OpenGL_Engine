#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;

uniform mat4 world;
uniform mat4 worldInvTranspose;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 normal;

void main()
{
    gl_Position = projection * view * world * vec4(vPos, 1.0);

    fragPos = (world * vec4(vPos, 1.0)).xyz;
    normal = mat3(worldInvTranspose) * vNormal;
}
