#version 430

struct Light
{
	vec3 position;
	vec3 ambientColor;
	vec3 diffuseColor;
	vec3 specularColor;
	float range;
	float specularExp;
};

const uint MAX_LIGHT_COUNT = 10u;

uniform mat4 modelMat; 
uniform mat4 viewMat;
uniform mat4 projMat; 
uniform Light[MAX_LIGHT_COUNT] lights;
uniform int lightCount;
uniform vec3 cameraPosition;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;

out vec3 color;

// These are point lights, so attenuate based on distance
float Attenuate(Light light, vec3 worldPos)
{
    float dist = distance(light.position, worldPos);
    float att = clamp(1.f - (dist * dist / (light.range * light.range)), 0.f, 1.f);
    return att * att;
}

void main() 
{
	vec3 resultColor = vec3(0);
	
	vec3 n = vec3(normalize(inverse(transpose(modelMat)) * vec4(vertex_normal, 0.0f)));
	vec4 worldPos = modelMat * vec4(vertex_position, 1.0f);
	vec3 viewVec = normalize(cameraPosition - worldPos.xyz);

	for (int i = 0; i < lightCount; i++)
	{
		vec3 lightDir = normalize(worldPos.xyz - lights[i].position);

		// diffuse component
		float diffuse = max(dot(n, -lightDir), 0.f);

		// specular component
		vec3 r = reflect(lightDir, n);
		float spec = pow(max(dot(r, viewVec), 0.f), lights[i].specularExp);

		vec3 lightCol = lights[i].ambientColor + (lights[i].diffuseColor * diffuse) + (lights[i].specularColor * spec);
		// Point lights should attenuate
		lightCol *= Attenuate(lights[i], worldPos.xyz);

		resultColor += lightCol;
	}
	color = resultColor;
	
	gl_Position = projMat * viewMat * worldPos;
}




