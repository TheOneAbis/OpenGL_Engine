#version 430

struct Light
{
	vec3 position;
	vec3 ambientColor;
	vec3 diffuseColor;
	vec3 specularColor;
	float specularExp;
};

const int maxLightCount = 100;

in vec4 normal;
in vec4 fragPos;

uniform Light[maxLightCount] lights; 

out vec4 frag_color;

void main() 
{
	vec3 n = normalize(normal).xyz;
	for (int i = 0; i < maxLightCount; i++)
	{
		vec3 lightDir = normalize(fragPos.xyz - lights[i].position);
		float intensity = max(dot(n, lightDir), 0.0);

		frag_color = vec4(intensity * lights[i].ambientColor, 1.0f);
	}
}