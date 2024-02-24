#version 450 core

// Each vertex takes up 3 texels in the buffer
// i = vertex position, i + 1 = vertex normal, i + 2 = vertex texcoord
layout (binding = 0) uniform samplerBuffer vertData;
layout (binding = 1) uniform isamplerBuffer indexData;

// Each world matrix takes up 4 texels in the buffer
// Each one corresponding to a column
layout (binding = 2) uniform samplerBuffer worldData;

uniform int indexCount;
uniform vec3 albedoColor; // actually the ambient color

uniform mat4 cameraWorld;
uniform float cameraFOV;
uniform float aspectRatio;

in vec2 screenPos;
out vec4 fragColor;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

// Casts a ray and returns the barycentric coords of the hit on the tri being tested against.
// Returns false if no hit
bool RaycastTri(Ray ray, vec3 p0, vec3 p1, vec3 p2, out vec3 result)
{
    float EPSILON = 0.0001f;

    // get tri edges sharing p0
    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;

    vec3 p = cross(ray.direction, e2);
    float det = dot(e1, p);

    // if determinant is near 0, ray lies in tri plane.
    if (det < EPSILON)
        return false;

    float f = (1.f / det);
    vec3 toRayOrigin = ray.origin - p0;

    // calculate U and test if it's within tri bounds
    result.x = f * dot(toRayOrigin, p);
    if (result.x < 0.f || result.x > 1.f) 
        return false;

    // calculate V and test if coord is within tri bounds
    vec3 q = cross(toRayOrigin, e1);
    result.y = f * dot(ray.direction, q);
    if (result.y < 0.f || result.x + result.y > 1.f) 
        return false;

    result.z = f * dot(e2, q);

    return true;
}

void main()
{
    vec4 rayDirLocal = vec4(screenPos.x * cameraFOV * aspectRatio, screenPos.y * cameraFOV, 1.f, 0.f);
    mat4 camInvTrans = inverse(transpose(cameraWorld));

    vec3 hitColor = albedoColor;

    Ray r;
    r.origin = (cameraWorld * vec4(0, 0, 0, 1)).xyz;
    r.direction = (camInvTrans * normalize(rayDirLocal)).xyz;

    // single tri is made of 3 vertices
    // thus, i += 3 vertices * 3 texels per vertex = 9 texels per triangle
    for (int i = 0; i < indexCount; i += 3)
    {
        // Get the verts that create this tri
        vec4 p0 = texelFetch(vertData, texelFetch(indexData, i + 0).x * 3);
        vec4 p1 = texelFetch(vertData, texelFetch(indexData, i + 1).x * 3);
        vec4 p2 = texelFetch(vertData, texelFetch(indexData, i + 2).x * 3);

        // Get the world matrix for this tri
        mat4 world = mat4(
            texelFetch(worldData, int(p0.w) + 0), 
            texelFetch(worldData, int(p0.w) + 1), 
            texelFetch(worldData, int(p0.w) + 2), 
            texelFetch(worldData, int(p0.w) + 3)
            );

        // Raycast from camera position through this pixel to see if it hits this tri
        vec3 hitPos;
        if (RaycastTri(r, (world * vec4(p0.xyz, 1)).xyz, (world * vec4(p1.xyz, 1)).xyz, (world * vec4(p2.xyz, 1)).xyz, hitPos))
        {
            hitColor = vec3(1, 1, 1);
            break;
        }
    }
    fragColor = vec4(hitColor, 1);
}