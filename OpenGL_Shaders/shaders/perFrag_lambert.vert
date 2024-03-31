#version 430

uniform mat4 modelMat;
uniform mat4 viewMat; 
uniform mat4 projMat; 
uniform vec3 lightPos;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;

out vec4 normal;
out vec4 fragPos;

void main() 
{
	normal = normalize(inverse(transpose(modelMat)) * vec4(vertex_normal, 0.0f));
	fragPos = modelMat * vec4(vertex_position, 1.0f);
	gl_Position = projMat * viewMat * fragPos;
}