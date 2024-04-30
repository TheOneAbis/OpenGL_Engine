#version 450 core

// constants
const uint LIGHT_TYPE_DIRECTIONAL = 0u;
const uint LIGHT_TYPE_POINT = 1u;
const uint LIGHT_TYPE_SPOT = 2u;
const uint MAX_SPECULAR_EXPONENT = 256u;
const uint MAX_LIGHT_COUNT = 10u;

struct Light
{
    uint Type;         // Light Type = 0, 1 or 2 (see above)
    vec3 Direction;    // Direction for Directional and Spot lights
    float Range;       // Attenuation for Point and Spot lights
    vec3 Position;     // Position for Point and Spot lights
    float Intensity;   // Light intensity
    vec3 Color;        // Light color
    float SpotFalloff; // Cone size for Spot Lights (unused)
};

// Lambertian Diffuse
float DiffuseBRDF(vec3 normal, vec3 dirToLight)
{
    return clamp(dot(normal, dirToLight), 0, 1);
}

// Specular from Phong
float SpecularBRDF(vec3 normal, vec3 lightDir, vec3 viewVector, float roughness, float roughScale)
{
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    return pow(clamp(dot(reflect(lightDir, normal), viewVector), 0, 1), specExponent) * roughScale;
}

vec3 Phong(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 colorTint, vec3 viewVec, float roughness, float roughnessScale)
{
    // Calculate diffuse and specular values
    float diffuse = DiffuseBRDF(normal, -lightDir);
    float spec = SpecularBRDF(normal, lightDir, viewVec, roughness, roughnessScale);

    // Cut the specular if the diffuse contribution is zero
    spec *= diffuse == 0.f ? 0.f : 1.f;

    return lightColor * colorTint * (diffuse + spec);
}

float Attenuate(Light light, vec3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = clamp(1.0f - (dist * dist / (light.Range * light.Range)), 0.f, 1.f);
    return att * att;
}

// -------------------------------------------------------- \\
// --     PHYSICALLY-BASED RENDERING (Cook-Torrence)     -- \\
// -------------------------------------------------------- \\

// The fresnel value for non-metals
const float NONMETAL_F0 = 0.04f;
const float MIN_ROUGHNESS = 0.0000001f;
const float PI = 3.14159265359f;

// Diffuse from energy conservation
vec3 DiffuseEnergyConserve(float diffuse, vec3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// Normal Distribution Trowbridge-Reitz
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(vec3 n, vec3 h, float roughness)
{
	// Pre-calculations
    float NdotH = clamp(dot(n, h), 0.f, 1.f);
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS);

	// ((n dot h)^2 * (a^2 - 1) + 1)
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
vec3 F_Schlick(vec3 v, vec3 h, vec3 f0)
{
    float VdotH = clamp(dot(v, h), 0.f, 1.f);
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring
float G_SchlickGGX(vec3 n, vec3 v, float roughness)
{
	// remapping
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = clamp(dot(n, v), 0.f, 1.f);

    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance Microfacet BRDF (Specular)
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
vec3 MicrofacetBRDF(vec3 n, vec3 l, vec3 v, float roughness, vec3 f0, out vec3 F_out)
{
    vec3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    vec3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
    F_out = F;

	// Final specular formula
    vec3 specularResult = (D * F * G) / 4.f;
    return specularResult * max(dot(n, l), 0);
}

vec3 CookTorrence(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 surfaceColor,
    vec3 viewVec, float lightIntensity, float roughness, float metalness, vec3 specColor)
{
    // Diffuse uses same formula as Phong
    float diffuse = DiffuseBRDF(normal, -lightDir);
    
    // Get specular color and fresnel result
    vec3 fresnel;
    vec3 spec = MicrofacetBRDF(normal, -lightDir, viewVec, roughness, specColor, fresnel);
    
    // Calculate diffuse with energy conservation
    vec3 balancedDiff = DiffuseEnergyConserve(diffuse, fresnel, metalness);
    
    // Final diffuse & specular combination
    return (balancedDiff * surfaceColor + spec) * lightIntensity * lightColor;
}

// Texture samplers
uniform sampler2D texture_diffuse[1];
uniform sampler2D texture_specular[1];
uniform sampler2D texture_normal[1];

// INPUTS, UNIFORM, OUTPUTS
in vec3 worldPos;
in vec3 viewPos;
in vec3 normal;
in vec3 viewNormal;
in vec2 texCoord;

uniform vec3 albedoColor;
uniform float roughness;
uniform float metallic;
uniform float emissive;
uniform vec3 ambient;

uniform vec3 cameraPosition;
uniform Light[MAX_LIGHT_COUNT] lights;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragViewNormal;
layout (location = 2) out vec3 fragViewPos;
layout (location = 3) out vec4 fragSpecular;

void main()
{
    fragViewPos = viewPos;
    fragViewNormal = normalize(viewNormal);

    vec3 viewVector = normalize(viewPos);

    // Get the actual base color from albedo and any textures
    vec3 baseColor = albedoColor * texture(texture_diffuse[0], texCoord).xyz;

    vec3 specularColor = mix(vec3(NONMETAL_F0, NONMETAL_F0, NONMETAL_F0), baseColor, metallic);
    fragSpecular = vec4(specularColor, roughness);

    vec3 totalLightColor = ambient * baseColor * (1 - metallic);
    totalLightColor += emissive.xxx;

    // Loop through the lights
    for (uint i = 0u; i < MAX_LIGHT_COUNT; i++)
    {
        vec3 lightDir;
        bool attenuate = false;
        switch (lights[i].Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                lightDir = normalize(lights[i].Direction);
                break;
            case LIGHT_TYPE_POINT:
                lightDir = normalize(worldPos - lights[i].Position);
                attenuate = true;
                break;
            case LIGHT_TYPE_SPOT:
                lightDir = normalize(worldPos - lights[i].Position);
                attenuate = true;
                break;
        }
        vec3 lightCol = CookTorrence(normalize(normal), lightDir, lights[i].Color, baseColor, viewVector, lights[i].Intensity, roughness, metallic, specularColor);

        // If this is a point or spot light, attenuate the color
        if (attenuate)
            lightCol *= Attenuate(lights[i], worldPos);
        
        totalLightColor += lightCol;
    }
    
    // return final color
    fragColor = totalLightColor;
}
