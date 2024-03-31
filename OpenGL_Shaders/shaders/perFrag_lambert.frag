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

in vec4 normal;
in vec4 fragPos;

uniform Light[MAX_LIGHT_COUNT] lights;
uniform int lightCount;
uniform vec3 cameraPosition;

out vec4 frag_color;

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
	
	vec3 n = normalize(normal.xyz);
	vec3 worldPos = fragPos.xyz;
	vec3 viewVec = normalize(cameraPosition - worldPos);

	for (int i = 0; i < lightCount; i++)
	{
		vec3 lightDir = normalize(worldPos - lights[i].position);

		// diffuse component
		float diffuse = max(dot(n, -lightDir), 0.f);

		// specular component
		vec3 r = reflect(lightDir, n);
		float spec = pow(max(dot(r, viewVec), 0.f), lights[i].specularExp);

		vec3 lightCol = lights[i].ambientColor + (lights[i].diffuseColor * diffuse) + (lights[i].specularColor * spec);
		// Point lights should attenuate
		lightCol *= Attenuate(lights[i], worldPos);

		resultColor += lightCol;
	}
	frag_color = vec4(resultColor, 1);
}