#version 430

in vec3 color;
in vec4 lightDir;
in vec4 normal;

out vec4 frag_color;

void main() {

  float intensity = 2.5* max(dot(normal, lightDir), 0.0);
  vec3 c;
  if (intensity > 0.95)
		c = 0.95 * color;
	else if (intensity > 0.5)
		c = 0.5 * color;
	else if (intensity > 0.25)
		c = 0.25 * color;
	else
		c = 0.05 * color;

  frag_color = vec4(c, 1.0f);
}