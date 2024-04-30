#version 450 core

#define MAX_ITERATION 2000
#define STEP_SIZE 2
#define MAX_THICKNESS 0.001f

// Texture samplers
layout (binding = 0) uniform sampler2D colorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D depthTexture;

// INPUTS, UNIFORM, OUTPUTS
uniform mat4 projection;
uniform mat4 view;

uniform vec4 skyColor;

out vec4 fragColor;

const float near = 0.1f;
const float far = 10.f;

void main()
{
    vec2 texSize = textureSize(colorTexture, 0).xy;

    // adding 0.5 moves position to center of the pixel area
    vec2 texCoord = (gl_FragCoord.xy + 0.5f) / texSize; // [0 - 1]

    vec4 color = texture(colorTexture, texCoord);
    vec4 normalRefl = texture(normalTexture, texCoord);
    vec4 worldNormal = vec4(normalize(normalRefl.xyz), 0);
    vec3 normal = (view * worldNormal).xyz;
    float reflMask = normalRefl.w;
    
    vec4 reflectionColor = vec4(0);
    if (reflMask != 0.f)
    {
        // compute position, reflection vec, max trace dist of this sample in tex space
        float depth = texture(depthTexture, texCoord).x;
        vec4 posInCS = vec4(texCoord * 2.f - 1.f, depth, 1.f) * -1.f;
        vec4 posInVS = inverse(projection) * posInCS;
        posInVS /= posInVS.w;

        vec3 camToPosVS = normalize(posInVS.xyz);
        vec4 reflectionVS = vec4(reflect(camToPosVS.xyz, normal), 0);

        vec4 reflEndPosVS = posInVS + reflectionVS * 1000.f;
        reflEndPosVS /= (reflEndPosVS.z < 0 ? reflEndPosVS.z : 1);
        
        vec4 reflEndPosCS = projection * vec4(reflEndPosVS.xyz, 1);
        reflEndPosCS /= reflEndPosCS.w;
        vec3 reflDir = normalize((reflEndPosCS - posInCS).xyz);

        // convert to texture space
        posInCS.xy *= vec2(0.5f, -0.5f);
        posInCS.xy += vec2(0.5f, 0.5f);
        vec3 posInTS = posInCS.xyz;
        reflDir.xy *= vec2(0.5f, -0.5f);

        // Calculate max distance before ray goes outside visible area
        float maxDist = reflDir.x >= 0 ? (1 - posInTS.x) / reflDir.x : -posInTS.x / reflDir.x;
        maxDist = min(maxDist, reflDir.y <0 ? (-posInTS.y / reflDir.y) : ((1 - posInTS.y) / reflDir.y));
        maxDist = min(maxDist, reflDir.z <0 ? (-posInTS.z / reflDir.z) : ((1 - posInTS.z) / reflDir.z));

        // find intersection in tex space by tracing reflection ray
        vec3 reflEndPosTS = posInTS + reflDir * maxDist;
        
        // calculate ray delta
        vec3 dp = reflEndPosTS - posInTS;
        ivec2 screenPos = ivec2(posInTS.xy * texSize);
        ivec2 endScreenPos = ivec2(reflEndPosTS.xy * texSize);
        ivec2 dp2 = endScreenPos - screenPos;
        const int max_dist = max(abs(dp2.x), abs(dp2.y));
        dp /= max_dist;

        vec4 rayPosInTS = vec4(posInTS.xyz + dp, 0);
        vec4 vRayDirInTS = vec4(dp.xyz, 0);
	    vec4 rayStartPos = rayPosInTS;

        int hitIndex = -1;
        for(int i = 0; i < max_dist && i < MAX_ITERATION; i += 4)
        {
            float depth = texture(depthTexture, rayPosInTS.xy).x;
	        float thickness = rayPosInTS.z - depth;
	        hitIndex = (thickness >= 0 && thickness < MAX_THICKNESS) ? i : hitIndex;

            if (hitIndex != -1) break;
		
            rayPosInTS += vRayDirInTS;
        }

        // calculate intersect position
        bool intersected = hitIndex >= 0;
        vec3 intersection = rayStartPos.xyz + vRayDirInTS.xyz * hitIndex;

        float intensity = intersected ? 1 : 0;

        // compute reflection vec if intersected
        reflectionColor = mix(skyColor, texture(colorTexture, intersection.xy), intensity);
    }
    // add reflection color to color of sample
    fragColor = color + reflectionColor;
}
