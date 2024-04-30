#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

uniform mat4 world;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 normal;
out vec2 texCoord;

void main()
{
    gl_Position = projection * view * world * vec4(vPos, 1.0);

    worldPos = (world * vec4(vPos, 1.f)).xyz;
    normal = inverse(transpose(mat3(world))) * vNormal;
    texCoord = vTexCoord;
}
