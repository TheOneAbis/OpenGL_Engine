#version 450 core

uniform vec3 albedoColor;

in vec3 screenPos;

// when writing to color, will actually write in Render Target 0
layout (location = 0) out vec3 color;

void main()
{
    color = vec3(1, 1, 0);
    if ((screenPos.x < 0 && screenPos.y < 0) || (screenPos.x > 0 && screenPos.y > 0))
        color.y = 0;
}
