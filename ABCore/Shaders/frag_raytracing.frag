#version 450 core

// constants
const uint LIGHT_TYPE_DIRECTIONAL = 0u;
const uint LIGHT_TYPE_POINT = 1u;
const uint LIGHT_TYPE_SPOT = 2u;
const float MAX_SPECULAR_EXPONENT = 256.f;
const uint MAX_LIGHT_COUNT = 10u;
const float EPSILON = 0.0001f;

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
    float reflectance;
    float diffuse;
    float refraction;
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

float SpecularBRDF(vec3 normal, vec3 lightDir, vec3 viewVector)
{
	// Get reflection of light bouncing off the surface 
    return pow(clamp(dot(reflect(lightDir, normal), viewVector), 0.f, 1.f), 20.f);
}

vec3 Phong(vec3 normal, vec3 lightDir, vec3 lightColor, vec3 colorTint, vec3 viewVec, float specScale, float diffuseScale)
{
    // Calculate diffuse and specular values
    float diffuse = DiffuseBRDF(normal, -lightDir) * diffuseScale;
    float spec = SpecularBRDF(normal, lightDir, viewVec) * specScale;

    // Cut the specular if the diffuse contribution is zero
    spec *= diffuse < EPSILON ? 0.f : 1.f;

    return lightColor * colorTint * (diffuse + spec);
}

float Attenuate(Light light, vec3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    return pow(clamp(1.0f - (dist * dist / (light.Range * light.Range)), 0.f, 1.f), 2);
}

vec3 GetBaryCoords(vec3 origin, vec3 dir, vec3 p0, vec3 p1, vec3 p2, out bool front)
{
    vec3 uvw = vec3(-1, -1, -1);

    // get tri edges sharing p0
    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;

    vec3 p = cross(dir, e2);
    float det = dot(e1, p);

    // if determinant is near 0, ray lies in tri plane.
    front = det > 0;
    if (abs(det) < EPSILON)
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
    bool front, thisfront;

    // loop through all tri's (every 3 indices)
    for (int i = 0; i < indexCount; i += 3)
    {
        ivec3 triIndices = ivec3(texelFetch(indexData, i + 0).x, texelFetch(indexData, i + 1).x, texelFetch(indexData, i + 2).x);

        // Get the first vert in this tri to get the world matrix
        vec4 p0 = texelFetch(vertData, triIndices.x * 4);
        mat4 world = mat4(
            texelFetch(worldData, int(p0.w) * 4 + 0), 
            texelFetch(worldData, int(p0.w) * 4 + 1), 
            texelFetch(worldData, int(p0.w) * 4 + 2), 
            texelFetch(worldData, int(p0.w) * 4 + 3));
        vec3 p0w = vec3(world * vec4(p0.xyz, 1));
        vec3 p1w = vec3(world * vec4(texelFetch(vertData, triIndices.y * 4).xyz, 1));
        vec3 p2w = vec3(world * vec4(texelFetch(vertData, triIndices.z * 4).xyz, 1));

        vec3 uvw = GetBaryCoords(origin, dir, p0w, p1w, p2w, thisfront);
        if (uvw.z > EPSILON && uvw.z < resultUVW.z)
        {
            front = thisfront;
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
        vec4 n = texelFetch(vertData, hitTriIndices.x * 4 + 1);
        vec3 n0 = worldinvtr * n.xyz;
        vec3 n1 = worldinvtr * texelFetch(vertData, hitTriIndices.y * 4 + 1).xyz;
        vec3 n2 = worldinvtr * texelFetch(vertData, hitTriIndices.z * 4 + 1).xyz;
                                               
        vec4 t0 = texelFetch(vertData, hitTriIndices.x * 4 + 2);
        vec4 t1 = texelFetch(vertData, hitTriIndices.y * 4 + 2);
        vec4 t2 = texelFetch(vertData, hitTriIndices.z * 4 + 2);

        hit.position = (1 - resultUVW.x - resultUVW.y) * finalp0w + resultUVW.x * finalp1w + resultUVW.y * finalp2w;
        hit.normal = normalize((1 - resultUVW.x - resultUVW.y) * n0 + resultUVW.x * n1 + resultUVW.y * n2) * (front ? 1 : -1);
        hit.texcoord = (1 - resultUVW.x - resultUVW.y) * t0 + resultUVW.x * t1 + resultUVW.y * t2;
        hit.object = object;
        hit.transmissive = n.w;

        vec4 extra = texelFetch(vertData, hitTriIndices.x * 4 + 3);
        hit.diffuse = extra.x;
        hit.reflectance = extra.y;
        hit.refraction = extra.z;
    }

    return successful;
}

// Casts a ray and returns if it hit something
bool Raycast(vec3 origin, vec3 dir, float maxDistance = 99999999.f)
{
    for (int i = 0; i < indexCount; i += 3)
    {
        vec4 p0 = texelFetch(vertData, texelFetch(indexData, i + 0).x * 4);
        mat4 world = mat4(
            texelFetch(worldData, int(p0.w) * 4 + 0), 
            texelFetch(worldData, int(p0.w) * 4 + 1), 
            texelFetch(worldData, int(p0.w) * 4 + 2), 
            texelFetch(worldData, int(p0.w) * 4 + 3)
            );
        vec3 p0w = vec3(world * vec4(p0.xyz, 1));
        vec3 p1w = vec3(world * vec4(texelFetch(vertData, texelFetch(indexData, i + 1).x * 4).xyz, 1));
        vec3 p2w = vec3(world * vec4(texelFetch(vertData, texelFetch(indexData, i + 2).x * 4).xyz, 1));

        bool junk;
        vec3 uvw = GetBaryCoords(origin, dir, p0w, p1w, p2w, junk);
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
    vec3 baseColor = hit.object == 2 ? GetFloorColor(hit.position) : hit.texcoord.xyz;
    float spec = 1 - clamp(hit.texcoord.w, 0.f, 0.99f);
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
            vec3 lightCol = Phong(hit.normal, lightDir, lights[i].Color, baseColor, viewVector, spec, hit.diffuse) * lights[i].Intensity; 
            if (attenuate) lightCol *= Attenuate(lights[i], hit.position);
            hitColor += lightCol;
        }
    }
    return hitColor;
}

struct StackStruct
{
    vec3 lightColor;
    vec3 origin;
    vec3 dir;
    float N;
    RaycastHit hit;
    int stackState;
};

void main()
{
    // set up initial values
    StackStruct[10] stack;
    stack[0].lightColor = vec3(0, 0, 0);
    stack[0].hit.reflectance = 1;
    stack[0].hit.transmissive = 1;
    stack[0].N = 1.f;
    stack[0].origin = cameraPos;
    stack[0].dir = normalize(vec3(inverse(view) * vec4(normalize(vec3(screenPos.x * cameraFOV * aspectRatio, screenPos.y * cameraFOV, -1.f)), 1)) - cameraPos);
    stack[0].stackState = 0; // 0 = reflection stage, 1 = transmission stage, 2 = return color stage
    stack[1] = stack[0];
    int count = 1; // track how deep in the stack we are
    
    do
    {
        if (count > recursionDepth)
        {
            count--;
            continue;
        }

        // pre-initialize the next stack frame before we start sending "parameters" to it
        stack[count + 1] = stack[count];
        stack[count + 1].stackState = 0;

        switch (stack[count].stackState)
        {
            case 0: // first time entering this stack frame
                if (Raycast(stack[count].origin, stack[count].dir, stack[count].hit))
                {
                    stack[count].stackState = 1;

                    // local illumination
                    stack[count].lightColor = LocalIlluminate(stack[count].origin, stack[count].hit);
                    stack[count].N = stack[count - 1].hit.refraction / stack[count].hit.refraction;

                    // reflection ray
                    if (stack[count].hit.reflectance > EPSILON)
                    {
                        // set the next frame's data (similar to passing in new params for recursive call)
                        stack[count + 1].dir = reflect(stack[count].dir, stack[count].hit.normal);
                        stack[count + 1].origin = stack[count].hit.position;
                        count++;
                    }
                }
                else
                {
                    // no hit; "return" the screen color
                    stack[count].lightColor = screenColor;
                    stack[count].stackState = 2;
                }
                break;

            case 1: // returning to this frame for transmission
                stack[count].stackState = 2;

                // transmission ray
                if (stack[count].hit.transmissive > EPSILON)
                {
                    stack[count + 1].dir = refract(stack[count].dir, stack[count].hit.normal, stack[count - 1].N);
                    stack[count + 1].origin = stack[count].hit.position;
                    count++;
                }
                break;

            case 2: // "return" the final color
                stack[count - 1].lightColor += stack[count].lightColor * 
                    (stack[count - 1].stackState == 1 ? stack[count - 1].hit.reflectance : 
                    stack[count - 1].hit.transmissive);
                count--;
                break;
        }
    } while (count > 0);
    
    // correct color w/ gamma and return final color
    fragColor = vec4(
        pow(stack[0].lightColor.x, 1.0f / 2.2f),
        pow(stack[0].lightColor.y, 1.0f / 2.2f),
        pow(stack[0].lightColor.z, 1.0f / 2.2f), 
        1);
}
