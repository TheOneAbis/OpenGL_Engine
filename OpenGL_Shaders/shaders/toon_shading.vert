#version 430

uniform mat4 modelMat;
uniform mat4 viewMat; 
uniform mat4 projMat; 
uniform vec3 lightPos;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;

out vec3 color;
out vec4 lightDir;
out vec4 normal;

void main() {
  vec4 lpos = viewMat * vec4(lightPos, 1.0f);
  vec4 ppos = viewMat * modelMat * vec4 (vertex_position, 1.0f);
  lightDir = normalize(lpos - ppos);

  normal = viewMat * modelMat * vec4(vertex_normal, 0.0f);
  color = vec3(1.0, 1.0, 0.0);
	
  gl_Position = projMat * viewMat * modelMat * vec4(vertex_position, 1.0f);
}




