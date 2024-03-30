#version 430
in float noise; 
out vec4 frag_color;

void main() {
  vec3 c;

  // The following color spectrum adheres to a catoonish color gradient of explosion
  if (noise > 0.7)
		c = vec3(0.3, 0.05, 0.05);
	else if (noise > 0.6)
		c = vec3(0.4, 0.15, 0.05);
	else if (noise > 0.45)
		c = vec3(0.9, 0.25, 0.05);
	else if (noise > 0.4)
		c = vec3(0.9, 0.65, 0.05);	
	else if (noise > 0.4)
		c = vec3(0.9, 0.85, 0.05);	
	else
		c = vec3(1.0, 1.0, 0.5);

  frag_color = vec4(c, 1.0f);
}