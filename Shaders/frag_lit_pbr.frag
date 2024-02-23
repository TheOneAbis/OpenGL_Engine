#version 330 core

// constants
const uint LIGHT_TYPE_DIRECTIONAL = 0u;
const uint LIGHT_TYPE_POINT = 1u;
const uint LIGHT_TYPE_SPOT = 2u;
const uint MAX_SPECULAR_EXPONENT = 256u;
const uint MAX_LIGHT_COUNT = 10u;

struct Light
{
    uint Type;          // Which kind of light? 0, 1 or 2 (see above)
    vec3 Direction;    // Directional and Spot lights need a direction
    float Range;       // Point and Spot lights have a max range for attenuation
    vec3 Position;     // Point and Spot lights have a position in space
    float Intensity;   // All lights need an intensity
    vec3 Color;        // All lights need a color
    float SpotFalloff; // Spot lights need a value to define their “cone” size
};

// Basic lighting functions
float DiffuseBRDF(vec3 normal, vec3 dirToLight)
{
    return clamp(dot(normal, dirToLight), 0.f, 1.f);
}

float SpecularBRDF(vec3 normal, vec3 lightDir, vec3 viewVector, float roughness, float roughnessScale)
{
	// Get reflection of light bouncing off the surface 
    vec3 refl = reflect(normal, lightDir);
    
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	// Compare reflection against view vector, raising it to
	// a very high power to ensure the falloff to zero is quick
    return pow(clamp(dot(refl, viewVector), 0.f, 1.f), specExponent) * roughnessScale;
}

vec3 ColorFromLight(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 colorTint, vec3 viewVec, float roughness, float roughnessScale)
{
    // Calculate diffuse an specular values
    float diffuse = DiffuseBRDF(normal, -lightDir);
    float spec = SpecularBRDF(normal, lightDir, viewVec, roughness, roughnessScale);
    
    // Cut the specular if the diffuse contribution is zero
    spec *= diffuse == 0 ? 0.f : diffuse;

    return lightColor * colorTint * diffuse + spec;
}

float Attenuate(Light light, vec3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = clamp(1.0f - (dist * dist / (light.Range * light.Range)), 0.f, 1.f);
    return att * att;
}

// -------------------------------------------------------- \\
// -- PHYSICALLY-BASED RENDERING FUNCTIONS AND CONSTANTS -- \\
// -------------------------------------------------------- \\

// The fresnel value for non-metals (dielectrics)
// Page 9: "F0 of nonmetals is now a constant 0.04"
const float NONMETAL_F0 = 0.04f;
const float MIN_ROUGHNESS = 0.0000001f;
const float PI = 3.14159265359f;

// Calculates diffuse amount based on energy conservation
//
// diffuse - Diffuse amount
// F - Fresnel result from microfacet BRDF
// metalness - surface metalness amount
vec3 DiffuseEnergyConserve(float diffuse, vec3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// Normal Distribution Function: GGX (Trowbridge-Reitz)
// a - Roughness
// h - Half vector: (V + L)/2
// n - Normal
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(vec3 n, vec3 h, float roughness)
{
	// Pre-calculations
    float NdotH = clamp(dot(n, h), 0.f, 1.f);
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	// Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
// v - View vector
// h - Half vector
// f0 - Value when l = n
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
vec3 F_Schlick(vec3 v, vec3 h, vec3 f0)
{
    float VdotH = clamp(dot(v, h), 0.f, 1.f);
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
// n - Normal
// v - View vector
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(vec3 n, vec3 v, float roughness)
{
	// End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = clamp(dot(n, v), 0.f, 1.f);

	// Final value
	// Note: Numerator should be NdotV (or NdotL depending on parameters).
	// However, these are also in the BRDF's denominator, so they'll cancel!
	// We're leaving them out here AND in the BRDF function as the
	// dot products can get VERY small and cause rounding errors.
    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance Microfacet BRDF (Specular)
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
vec3 MicrofacetBRDF(vec3 n, vec3 l, vec3 v, float roughness, vec3 f0, out vec3 F_out)
{
    vec3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    vec3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
    F_out = F;

	// Final specular formula
	// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
	// canceled out by our G() term.  As such, they have been removed
	// from BOTH places to prevent floating point rounding errors.
    vec3 specularResult = (D * F * G) / 4.f;

	// One last non-obvious requirement: According to the rendering equation,
	// specular must have the same NdotL applied as diffuse!  We'll apply
	// that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}



vec3 ColorFromLightPBR(vec3 normal, vec3 toLight, vec3 lightColor, vec3 surfaceColor,
    vec3 viewVec, float lightIntensity, float roughness, float metalness, vec3 specColor)
{
    // Diffuse is unchanged from non-PBR
    float diffuse = DiffuseBRDF(normal, toLight);
    
    vec3 fresnel;
    // Get specular color and fresnel result
    vec3 spec = MicrofacetBRDF(normal, toLight, viewVec, roughness, specColor, fresnel);
    
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    vec3 balancedDiff = DiffuseEnergyConserve(diffuse, fresnel, metalness);
    
    // Combine the final diffuse and specular values for this light
    return (balancedDiff * surfaceColor + spec) * lightIntensity * lightColor;
}

// INPUTS, UNIFORM, OUTPUTS
in vec3 worldPos;
in vec3 normal;
in vec2 texCoord;

uniform vec3 albedoColor;
uniform float roughness;
uniform float metallic;
uniform vec3 ambient;

uniform vec3 cameraPosition;
uniform Light[MAX_LIGHT_COUNT] lights;

out vec4 fragColor;

void main()
{
    vec3 newNormal = normalize(normal);
    vec3 viewVector = normalize(cameraPosition - worldPos);

    vec3 specularColor = mix(vec3(NONMETAL_F0, NONMETAL_F0, NONMETAL_F0), albedoColor.rgb, metallic);

    vec3 totalLightColor = ambient * albedoColor * (1 - metallic);

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
        vec3 lightCol = ColorFromLightPBR(newNormal, -lightDir, lights[i].Color, albedoColor, viewVector, lights[i].Intensity, roughness, metallic, specularColor);

        // If this is a point or spot light, attenuate the color
        if (attenuate)
            lightCol *= Attenuate(lights[i], worldPos);
        
        totalLightColor += lightCol;
    }
    
    // correct color w/ gamma and return final color
    
    fragColor = vec4(pow(totalLightColor.x, 1.0f / 2.2f),
                     pow(totalLightColor.y, 1.0f / 2.2f),
                     pow(totalLightColor.z, 1.0f / 2.2f), 1);
}
