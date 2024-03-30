#version 430

uniform mat4 modelMat; 
uniform mat4 viewMat;
uniform mat4 projMat; 
uniform vec3 lightPos;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;

out vec3 color;

void main() {
  
  vec4 lpos = viewMat * vec4(lightPos, 1.0f);
  vec4 ppos = viewMat * modelMat * vec4 (vertex_position, 1.0f);
  vec4 lightDir = normalize(lpos - ppos);
  
  vec4 normal = normalize (viewMat * modelMat * vec4(vertex_normal, 0.0f));
  
  float intensity = max(dot(normal, lightDir), 0.0);
  
  color = intensity * vec3(1.0, 1.0, 0.0);
	
  gl_Position = projMat * viewMat * modelMat * vec4(vertex_position, 1.0f);
}




