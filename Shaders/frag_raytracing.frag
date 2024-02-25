#version 450 core

// constants
const uint LIGHT_TYPE_DIRECTIONAL = 0u;
const uint LIGHT_TYPE_POINT = 1u;
const uint LIGHT_TYPE_SPOT = 2u;
const uint MAX_SPECULAR_EXPONENT = 256u;
const uint MAX_LIGHT_COUNT = 10u;

struct Light
{
    uint Type;         // Which kind of light? 0, 1 or 2 (see above)
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
    return clamp(dot(normal, dirToLight), 0, 1);
}

float SpecularBRDF(vec3 normal, vec3 lightDir, vec3 viewVector, float roughness, float roughScale)
{
	// Get reflection of light bouncing off the surface 
    vec3 refl = reflect(lightDir, normal);
    
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	// Compare reflection against view vector, raising it to
	// a very high power to ensure the falloff to zero is quick
    return pow(clamp(dot(refl, viewVector), 0, 1), specExponent) * roughScale;
}

vec3 ColorFromLight(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 colorTint, vec3 viewVec, float roughness, float roughnessScale)
{
    // Calculate diffuse and specular values
    float diffuse = DiffuseBRDF(normal, -lightDir);
    float spec = SpecularBRDF(normal, lightDir, viewVec, roughness, roughnessScale);

    // Cut the specular if the diffuse contribution is zero
    spec *= diffuse == 0.f ? 0.f : 1.f;

    return lightColor * colorTint * diffuse + spec;
}

float Attenuate(Light light, vec3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = clamp(1.0f - (dist * dist / (light.Range * light.Range)), 0.f, 1.f);
    return att * att;
}

// -------------------------------------------------------- \\
// -- PHYSICALLY-BASED RENDERING FUNCTIONS AND CONSTANTS (Cook-Torrence) -- \\
// -------------------------------------------------------- \\

// The fresnel value for non-metals (dielectrics)
const float NONMETAL_F0 = 0.04f;
const float MIN_ROUGHNESS = 0.0000001f;
const float PI = 3.14159265359f;

// Calculates diffuse amount based on energy conservation
vec3 DiffuseEnergyConserve(float diffuse, vec3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// Normal Distribution Function: GGX (Trowbridge-Reitz)
float D_GGX(vec3 n, vec3 h, float roughness)
{
    float NdotH = clamp(dot(n, h), 0.f, 1.f);
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS);

	// ((n dot h)^2 * (a^2 - 1) + 1)
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
vec3 F_Schlick(vec3 v, vec3 h, vec3 f0)
{
    float VdotH = clamp(dot(v, h), 0.f, 1.f);
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
float G_SchlickGGX(vec3 n, vec3 v, float roughness)
{
	// remapping
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = clamp(dot(n, v), 0.f, 1.f);

	// Final value
    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance Microfacet BRDF (Specular)
vec3 MicrofacetBRDF(vec3 n, vec3 l, vec3 v, float roughness, vec3 f0, out vec3 F_out)
{
    vec3 h = normalize(v + l);

	// Run numerator functions
    float D = D_GGX(n, h, roughness);
    vec3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);
	
    F_out = F;

	// Final specular formula
    vec3 specularResult = (D * F * G) / 4;

	// According to the rendering equation,
	// specular must have the same NdotL applied as diffuse
    return specularResult * max(dot(n, l), 0);
}

vec3 ColorFromLightPBR(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 surfaceColor,
    vec3 viewVec, float roughness, float metalness, vec3 specColor)
{
    float diffuse = DiffuseBRDF(normal, -lightDir);
    
    vec3 fresnel;
    // Get specular color and fresnel result
    vec3 spec = MicrofacetBRDF(normal, -lightDir, viewVec, roughness, specColor, fresnel);
    
    // Calculate diffuse with energy conservation, including cutting diffuse for metals
    vec3 balancedDiff = DiffuseEnergyConserve(diffuse, fresnel, metalness);
    
    // Combine the final diffuse and specular values for this light
    return (balancedDiff * surfaceColor + spec) * lightColor;
}

#define EPSILON 0.0001f

// Casts a ray and returns the barycentric coords of the hit on the tri being tested against.
// Returns false if no hit
bool RaycastTri(vec3 origin, vec3 dir, vec3 p0, vec3 p1, vec3 p2, out vec3 result)
{
    // get tri edges sharing p0
    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;

    vec3 p = cross(dir, e2);
    float det = dot(e1, p);

    // if determinant is near 0, ray lies in tri plane.
    if (det < EPSILON)
        return false;

    float f = (1.f / det);
    vec3 toOrigin = origin - p0;
    // calculate U and test if it's within tri bounds
    result.x = f * dot(toOrigin, p);
    if (result.x < 0.f || result.x > 1.f) 
        return false;

    // calculate V and test if coord is within tri bounds
    vec3 q = cross(toOrigin, e1);
    result.y = f * dot(dir, q);
    if (result.y < 0.f || result.x + result.y > 1.f) 
        return false;

    result.z = f * dot(e2, q);
    return true;
}

// Each vertex takes up 3 texels in the buffer
// i = vertex position, i + 1 = vertex normal, i + 2 = vertex texcoord
layout (binding = 0) uniform samplerBuffer vertData;
layout (binding = 1) uniform isamplerBuffer indexData;

// Each world matrix takes up 4 texels in the buffer
// Each one corresponding to a column
layout (binding = 2) uniform samplerBuffer worldData;

uniform int indexCount;

// lighting params
uniform Light[MAX_LIGHT_COUNT] lights;
uniform vec3 ambient;
uniform vec3 screenColor;

uniform mat4 view;
uniform float cameraFOV;
uniform float aspectRatio;
uniform vec3 cameraPos;

in vec2 screenPos;
out vec4 fragColor;

void main()
{
    vec4 hitColor = vec4(screenColor, 1);
    vec4 rayDir = inverse(view) * vec4(normalize(vec3(screenPos.x * cameraFOV * aspectRatio, screenPos.y * cameraFOV, -1.f)), 1);
    float nearest = 999999999.f;

    // single tri is made of 3 vertices
    // thus, i += 3 vertices * 3 texels per vertex = 9 texels per triangle
    for (int i = 0; i < indexCount; i += 3)
    {
        int i0 = texelFetch(indexData, i + 0).x;
        int i1 = texelFetch(indexData, i + 1).x;
        int i2 = texelFetch(indexData, i + 2).x;
        // Get the first vert in this tri to get the world matrix
        vec4 p0 = texelFetch(vertData, i0 * 3);
        mat4 world = mat4(
            texelFetch(worldData, int(p0.w) * 4 + 0), 
            texelFetch(worldData, int(p0.w) * 4 + 1), 
            texelFetch(worldData, int(p0.w) * 4 + 2), 
            texelFetch(worldData, int(p0.w) * 4 + 3)
            );
        vec3 p0w = vec3(world * vec4(p0.xyz, 1));
        vec3 p1w = vec3(world * vec4(texelFetch(vertData, i1 * 3).xyz, 1));
        vec3 p2w = vec3(world * vec4(texelFetch(vertData, i2 * 3).xyz, 1));

        // Raycast from camera position through this pixel to see if it hits this tri
        vec3 hit;
        if (RaycastTri(cameraPos, rayDir.xyz - cameraPos, p0w, p1w, p2w, hit))
        {
            if (hit.z > EPSILON && hit.z < nearest)
            {
                nearest = hit.z;

                // Get interpolated normal and tex coord
                mat3 worldinvtr = mat3(inverse(transpose(world)));
                vec3 n0 = worldinvtr * texelFetch(vertData, i0 * 3 + 1).xyz;
                vec3 n1 = worldinvtr * texelFetch(vertData, i1 * 3 + 1).xyz;
                vec3 n2 = worldinvtr * texelFetch(vertData, i2 * 3 + 1).xyz;
                                               
                vec3 t0 = texelFetch(vertData, i0 * 3 + 2).xyz;
                //vec2 t1 = texelFetch(vertData, i1 * 3 + 2).xy;
                //vec2 t2 = texelFetch(vertData, i2 * 3 + 2).xy;

                vec3 worldPos = (1 - hit.x - hit.y) * p0w + hit.x * p1w + hit.y * p2w;
                vec3 normal = normalize((1 - hit.x - hit.y) * n0 + hit.x * n1 + hit.y * n2);
                //vec2 texcoord = (1 - hit.x - hit.y) * t0 + hit.x * t1 + hit.y * t2;
           
                // Calculate lighting at the hit
                vec3 baseColor = t0;
                float placeholderRough = 0.4f;
                float placeholderMetal = 0.75f;

                vec3 viewVector = normalize(cameraPos - worldPos);
                vec3 specularColor = mix(vec3(NONMETAL_F0), baseColor, vec3(placeholderMetal));
                vec3 totalLightColor = ambient * baseColor * (1 - placeholderMetal);
                
                // Loop through the lights
                for (uint i = 0u; i < MAX_LIGHT_COUNT; i++)
                {
                    vec3 lightDir;
                    bool attenuate = false;
                    switch (lights[i].Type)
                    {
                        case LIGHT_TYPE_DIRECTIONAL:
                            lightDir = lights[i].Direction;
                            break;
                        default:
                            lightDir = worldPos - lights[i].Position;
                            attenuate = true;
                            break;
                    }

                    // shadow ray for each light
                    //I AM VIOLENTLY CRYING OVER THESE FOR LOOPS
                    bool hit = false;
                    float lightDist = length(lightDir);
                    lightDir = normalize(lightDir);
                    for (int i = 0; i < indexCount; i += 3)
                    {
                        vec4 p0 = texelFetch(vertData, texelFetch(indexData, i + 0).x * 3);
                        mat4 world = mat4(
                            texelFetch(worldData, int(p0.w) * 4 + 0), 
                            texelFetch(worldData, int(p0.w) * 4 + 1), 
                            texelFetch(worldData, int(p0.w) * 4 + 2), 
                            texelFetch(worldData, int(p0.w) * 4 + 3)
                            );
                        vec3 p0w = vec3(world * vec4(p0.xyz, 1));
                        vec3 p1w = vec3(world * vec4(texelFetch(vertData, texelFetch(indexData, i + 1).x * 3).xyz, 1));
                        vec3 p2w = vec3(world * vec4(texelFetch(vertData, texelFetch(indexData, i + 2).x * 3).xyz, 1));

                        // Raycast from camera position through this pixel to see if it hits this tri
                        vec3 shadowHit;
                        if (RaycastTri(worldPos, -lightDir, p0w, p1w, p2w, shadowHit))
                        {
                            if (shadowHit.z > EPSILON && shadowHit.z < lightDist)
                            {
                                hit = true;
                                break;
                            }
                        }
                    }
                    if (!hit) // no hits detected, do the lighting thing
                    {
                        vec3 lightCol = ColorFromLightPBR( // Cook-Torrence
                            normal, 
                            lightDir, 
                            lights[i].Color, 
                            baseColor, 
                            viewVector,
                            placeholderRough, 
                            placeholderMetal, 
                            specularColor) * lights[i].Intensity; 
//                        vec3 lightCol = ColorFromLight( // Phong
//                            normal, 
//                            lightDir, 
//                            lights[i].Color, 
//                            baseColor, 
//                            viewVector, 
//                            placeholderRough, 
//                            1 - placeholderRough) * lights[i].Intensity; 
                    
                        // If this is a point or spot light, attenuate the color
                        if (attenuate)
                            lightCol *= Attenuate(lights[i], worldPos);

                        totalLightColor += lightCol;
                    }
                } // end light loop

                // correct color w/ gamma and return final color
                hitColor = vec4(pow(totalLightColor.x, 1.0f / 2.2f),
                                pow(totalLightColor.y, 1.0f / 2.2f),
                                pow(totalLightColor.z, 1.0f / 2.2f), 1);
                break;
            }
        }
    } // end original raycast
    fragColor = hitColor;
}
