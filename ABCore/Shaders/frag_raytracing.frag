#version 450 core

// constants
const uint LIGHT_TYPE_DIRECTIONAL = 0u;
const uint LIGHT_TYPE_POINT = 1u;
const uint LIGHT_TYPE_SPOT = 2u;
const float MAX_SPECULAR_EXPONENT = 256.f;
const uint MAX_LIGHT_COUNT = 10u;
const float EPSILON = 0.0001f;
const float GLASS_REFRACTION = 0.95f;

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

struct RaycastHit
{
    vec3 position;
    vec3 normal;
    vec4 texcoord;
    int object;
    float transmissive;
    vec3 color;
    int reflIndex;
    int refrIndex;
};

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
uniform int recursionDepth;

uniform mat4 view;
uniform float cameraFOV;
uniform float aspectRatio;
uniform vec3 cameraPos;

in vec2 screenPos;
out vec4 fragColor;

// Basic lighting functions
float DiffuseBRDF(vec3 normal, vec3 dirToLight)
{
    return clamp(dot(normal, dirToLight), 0.f, 1.f);
}

float SpecularBRDF(vec3 normal, vec3 lightDir, vec3 viewVector, float roughness)
{
	// Get reflection of light bouncing off the surface 
    float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
    return pow(clamp(dot(reflect(lightDir, normal), viewVector), 0.f, 1.f), specExponent);
}

vec3 Phong(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 colorTint, vec3 viewVec, float roughness)
{
    // Calculate diffuse and specular values
    float diffuse = DiffuseBRDF(normal, -lightDir);
    float spec = SpecularBRDF(normal, lightDir, viewVec, roughness);

    // Cut the specular if the diffuse contribution is zero
    spec *= diffuse < EPSILON ? 0.f : 1.f;

    return lightColor * colorTint * (diffuse + spec);
}

float Attenuate(Light light, vec3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    return pow(clamp(1.0f - (dist * dist / (light.Range * light.Range)), 0.f, 1.f), 2);
}

vec3 GetBaryCoords(vec3 origin, vec3 dir, vec3 p0, vec3 p1, vec3 p2)
{
    vec3 uvw = vec3(-1, -1, -1);

    // get tri edges sharing p0
    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;

    vec3 p = cross(dir, e2);
    float det = dot(e1, p);

    // if determinant is near 0, ray lies in tri plane.
    if (det < EPSILON)
        return uvw;

    float f = (1.f / det);
    vec3 toOrigin = origin - p0;
    // calculate U and test if it's within tri bounds
    uvw.x = f * dot(toOrigin, p);
    if (uvw.x < 0.f || uvw.x > 1.f) 
        return uvw;

    // calculate V and test if coord is within tri bounds
    vec3 q = cross(toOrigin, e1);
    uvw.y = f * dot(dir, q);
    if (uvw.y < 0.f || uvw.x + uvw.y > 1.f) 
        return uvw;

    // calculate distance
    uvw.z = f * dot(e2, q);
    return uvw;
}

// Casts a ray and returns the barycentric coords of the hit on the tri being tested against.
// Returns false if no hit
bool Raycast(vec3 origin, vec3 dir, out RaycastHit hit)
{
    bool successful = false;
    vec3 resultUVW = vec3(-1, -1, 99999999.f);
    vec3 finalp0w, finalp1w, finalp2w;
    mat4 hitWorld;
    ivec3 hitTriIndices;
    int object;

    // loop through all tri's (every 3 indices)
    for (int i = 0; i < indexCount; i += 3)
    {
        ivec3 triIndices = ivec3(texelFetch(indexData, i + 0).x, texelFetch(indexData, i + 1).x, texelFetch(indexData, i + 2).x);

        // Get the first vert in this tri to get the world matrix
        vec4 p0 = texelFetch(vertData, triIndices.x * 3);
        mat4 world = mat4(
            texelFetch(worldData, int(p0.w) * 4 + 0), 
            texelFetch(worldData, int(p0.w) * 4 + 1), 
            texelFetch(worldData, int(p0.w) * 4 + 2), 
            texelFetch(worldData, int(p0.w) * 4 + 3));
        vec3 p0w = vec3(world * vec4(p0.xyz, 1));
        vec3 p1w = vec3(world * vec4(texelFetch(vertData, triIndices.y * 3).xyz, 1));
        vec3 p2w = vec3(world * vec4(texelFetch(vertData, triIndices.z * 3).xyz, 1));

        vec3 uvw = GetBaryCoords(origin, dir, p0w, p1w, p2w);
        if (uvw.z > EPSILON && uvw.z < resultUVW.z)
        {
            resultUVW = uvw;
            hitWorld = world;
            hitTriIndices = triIndices;
            finalp0w = p0w;
            finalp1w = p1w;
            finalp2w = p2w;
            object = int(p0.w);
            successful = true;
        }
    }

    // If found the barycentric coords of the tri hit, 
    // interpolate to find position, normal, and texcoords
    if (successful)
    {
        mat3 worldinvtr = mat3(inverse(transpose(hitWorld)));
        vec4 n = texelFetch(vertData, hitTriIndices.x * 3 + 1);
        vec3 n0 = worldinvtr * n.xyz;
        vec3 n1 = worldinvtr * texelFetch(vertData, hitTriIndices.y * 3 + 1).xyz;
        vec3 n2 = worldinvtr * texelFetch(vertData, hitTriIndices.z * 3 + 1).xyz;
                                               
        vec4 t0 = texelFetch(vertData, hitTriIndices.x * 3 + 2);
        vec4 t1 = texelFetch(vertData, hitTriIndices.y * 3 + 2);
        vec4 t2 = texelFetch(vertData, hitTriIndices.z * 3 + 2);

        hit.position = (1 - resultUVW.x - resultUVW.y) * finalp0w + resultUVW.x * finalp1w + resultUVW.y * finalp2w;
        hit.normal = normalize((1 - resultUVW.x - resultUVW.y) * n0 + resultUVW.x * n1 + resultUVW.y * n2);
        hit.texcoord = (1 - resultUVW.x - resultUVW.y) * t0 + resultUVW.x * t1 + resultUVW.y * t2;
        hit.object = object;
        hit.transmissive = n.w;
    }

    return successful;
}

// Casts a ray and returns if it hit something
bool Raycast(vec3 origin, vec3 dir, float maxDistance = 99999999.f)
{
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

        vec3 uvw = GetBaryCoords(origin, dir, p0w, p1w, p2w);
        if (uvw.z > EPSILON && uvw.z < maxDistance)
            return true;
    }
    return false;
}

vec3 GetFloorColor(vec3 worldPos)
{
    vec3 color = vec3(1, 1, 0);

    ivec2 scaledPos = abs(ivec3(worldPos * 4) % 2).xz;
    if (scaledPos == ivec2(0, 0) || scaledPos == ivec2(1, 1))
        color.y = 0;
    if (worldPos.x < 0)
        color.y = 1 - color.y;

    return color;
}

vec3 LocalIlluminate(vec3 origin, RaycastHit hit)
{
    vec3 baseColor = hit.object == 3 ? GetFloorColor(hit.position) : hit.texcoord.xyz;
    float rough = clamp(hit.texcoord.w, 0.f, 0.99f);
    vec3 viewVector = normalize(origin - hit.position);

    // Calculate lighting at the hit
    vec3 hitColor = ambient * baseColor;
                
    // Loop through the lights
    for (uint i = 0u; i < MAX_LIGHT_COUNT; i++)
    {
        vec3 lightDir = lights[i].Type == LIGHT_TYPE_DIRECTIONAL ? lights[i].Direction * 500.f : hit.position - lights[i].Position;
        bool attenuate = lights[i].Type != LIGHT_TYPE_DIRECTIONAL;

        // shadow ray for each light; do lighting if no hit
        if (!Raycast(hit.position, -lightDir, length(lightDir)))
        {
            lightDir = normalize(lightDir);
            vec3 lightCol = Phong(hit.normal, lightDir, lights[i].Color, baseColor, viewVector, rough) * lights[i].Intensity; 
            if (attenuate) lightCol *= Attenuate(lights[i], hit.position);
            hitColor += lightCol;
        }
    }
    return hitColor;
}

void main()
{
    // set up initial values
    vec3 totalLightColor = screenColor;
    vec3 origin = cameraPos;
    vec3 rayDir = normalize(vec3(inverse(view) * vec4(normalize(vec3(screenPos.x * cameraFOV * aspectRatio, screenPos.y * cameraFOV, -1.f)), 1)) - origin);
    float kr = 1.f;
    float kt = 1.f;
    float N = 1, prevN;
    RaycastHit currentHit, firstHit;

    // start w/ primary ray
    if (Raycast(origin, rayDir, firstHit))
    {
        totalLightColor = LocalIlluminate(origin, firstHit);
        kr *= 1 - firstHit.texcoord.w;
        kt *= firstHit.transmissive;

        origin = firstHit.position;
        vec3 reflDir = reflect(rayDir, firstHit.normal);
        currentHit = firstHit;

        // Reflection rays
        while (true)
        {
            if (Raycast(origin, reflDir, currentHit))
            {
                totalLightColor += kr * LocalIlluminate(origin, currentHit);

                if (kr > EPSILON)
                {
                    reflDir = reflect(reflDir, currentHit.normal);
                    origin = currentHit.position;
                    kr *= 1 - currentHit.texcoord.w;
                    continue;
                }
                break;
            }
            // Ray hit nothing; add ambient
            else
            {
                totalLightColor += kr * screenColor;
                break;
            }
        }

        origin = firstHit.position;
        prevN = N;
        N = firstHit.object == 0 ? GLASS_REFRACTION : 1.f;
        vec3 refrDir = refract(rayDir, firstHit.normal, N / prevN);
        currentHit = firstHit;

        // Transmission rays
        while (true)
        {
            if (Raycast(origin, refrDir, currentHit))
            {
                totalLightColor += kt * LocalIlluminate(origin, currentHit);

                if (kt > EPSILON)
                {
                    if (currentHit.object == 0)
                    {
                        float temp = N;
                        N = prevN == GLASS_REFRACTION ? 1.f : GLASS_REFRACTION;
                        prevN = temp;
                    }
                    
                    rayDir = refract(rayDir, currentHit.normal, N / prevN);
                    origin = currentHit.position;
                    kt *= currentHit.transmissive;
                    continue;
                }
                break;
            }
            // Ray hit nothing; add ambient
            else
            {
            
                totalLightColor += kt * screenColor;
                break;
            }
        }
    }
    
    // correct color w/ gamma and return final color
    fragColor = vec4(
        pow(totalLightColor.x, 1.0f / 2.2f),
        pow(totalLightColor.y, 1.0f / 2.2f),
        pow(totalLightColor.z, 1.0f / 2.2f), 
        1);
}
