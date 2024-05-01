#version 450 core

#define MAX_ITERATION 2000
#define MAX_THICKNESS 0.0001f

// Texture samplers
layout (binding = 0) uniform sampler2D colorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D depthTexture;

// INPUTS, UNIFORM, OUTPUTS
uniform mat4 projection;
uniform mat4 view;
uniform vec4 ambient;

out vec4 fragColor;

void main()
{
    vec2 texSize = textureSize(colorTexture, 0).xy;

    vec2 texCoord = (gl_FragCoord.xy) / texSize; // [0 - 1]

    vec4 color = texture(colorTexture, texCoord);
    vec4 normalData = texture(normalTexture, texCoord);
    vec3 normal = mat3(view) * normalData.xyz;
    
    vec4 reflectionColor = vec4(0);
    if (normalData.w != 0.f)
    {
        // compute position, reflection vec, max trace dist of this sample in tex space
        vec3 clipStart = vec3(texCoord * 2.f - 1.f, texture(depthTexture, texCoord).x);
        vec4 posInVS = inverse(projection) * vec4(clipStart, 1);
        vec3 viewStart = posInVS.xyz / posInVS.w;

        vec3 viewEnd = viewStart + reflect(normalize(viewStart), normal) * 1000.f;
        viewEnd /= (viewEnd.z > 0 ? viewEnd.z : 1);
        
        vec4 clipEnd = projection * vec4(viewEnd, 1);
        clipEnd.xyz /= clipEnd.w;
        vec3 reflDir = normalize(clipEnd.xyz - clipStart);

        // convert to texture space
        vec3 texStart = clipStart;
        texStart.xy *= vec2(0.5f, 0.5f);
        texStart.xy += vec2(0.5f, 0.5f);
        reflDir.xy *= vec2(0.5f, 0.5f);

        // Calculate max distance before ray goes outside visible area
        float maxDist = reflDir.x >= 0 ? (1 - texStart.x) / reflDir.x : -texStart.x / reflDir.x;
        maxDist = min(maxDist, reflDir.y < 0 ? (-texStart.y / reflDir.y) : ((1 - texStart.y) / reflDir.y));
        maxDist = min(maxDist, reflDir.z < 0 ? (-texStart.z / reflDir.z) : ((1 - texStart.z) / reflDir.z));

        // find intersection in tex space by tracing reflection ray
        vec3 texEnd = texStart + reflDir * maxDist;
        
        // calculate ray delta
        vec3 dp = texEnd - texStart;
        ivec2 screenPos = ivec2(texStart.xy * texSize);
        ivec2 endScreenPos = ivec2(texEnd.xy * texSize);
        ivec2 dp2 = endScreenPos - screenPos;
        const int max_dist = max(abs(dp2.x), abs(dp2.y));
        dp /= max_dist;

        vec3 texPos = texStart.xyz + dp;
        vec3 texDir = dp.xyz;
	    vec3 rayStartPos = texPos;

        int hitIndex = -1;
        for(int i = 0; i < max_dist && i < MAX_ITERATION; i++)
        {
            float depth = texture(depthTexture, texPos.xy).x;
	        float thickness = texPos.z - depth;
	        hitIndex = (thickness >= 0 && thickness < MAX_THICKNESS) ? i : hitIndex;

            if (hitIndex != -1) break;
		
            texPos += texDir;
        }

        // calculate intersect position
        vec3 intersection = rayStartPos + texDir * hitIndex;
        float intensity = hitIndex >= 0 ? 1.f : 0.f;

        // compute reflection vec if intersected
        reflectionColor = mix(ambient, texture(colorTexture, intersection.xy), intensity) * normalData.w;
    }

    // add reflection color to color of sample
    fragColor = color + reflectionColor;
}
