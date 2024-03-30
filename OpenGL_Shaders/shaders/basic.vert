#version 330 core 
uniform mat4 modelMat; 
uniform mat4 viewMat; 
uniform mat4 projMat;
uniform float offset;

layout (location = 0) in vec3 aPos; 
layout (location = 1) in vec3 aNorm; 
out vec4 vColor; // color output to the fragment shader 
void main() 
{ 
	vec3 p = aPos + aNorm * offset;
	gl_Position = projMat * viewMat * modelMat * vec4(p, 1.0); 
	vColor = vec4((aNorm+1.0f)/2.0f, 1.0);
	
}
