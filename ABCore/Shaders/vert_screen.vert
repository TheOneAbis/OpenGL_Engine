#version 450 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out vec2 screenPos;

// Vertices are already where they should be, so just pass them through
// (This shader should be used for postprocess / raytracing stuff)
void main()
{
    gl_Position = vec4(vPos, 1.0);
    screenPos = vPos.xy;
}
