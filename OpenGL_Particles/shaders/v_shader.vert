#version 430

uniform mat4 viewMat; 
uniform mat4 projMat; 

layout(location = 0) in vec4 vertex_position;
layout(location = 1) in vec4 vertex_colour;

out vec4 colour;

void main() {
  colour = vertex_colour;
  gl_Position = projMat * viewMat * vertex_position;
}




