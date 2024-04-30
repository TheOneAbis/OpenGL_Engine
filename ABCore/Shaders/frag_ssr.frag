#version 450 core

// Texture samplers
layout (binding = 0) uniform sampler2D colorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D positionTexture;
layout (binding = 3) uniform sampler2D specularTexture;

// INPUTS, UNIFORM, OUTPUTS
uniform mat4 projection;
uniform mat4 view;

uniform float maxDistance;
uniform float resolution;
uniform int steps;
uniform float thickness;

out vec4 fragColor;

void main()
{
    // get texture dimensions and the coord of this pixel
    vec2 texSize = textureSize(positionTexture, 0).xy;
    vec2 texCoord = gl_FragCoord.xy / texSize;

    vec4 uv = vec4(0.f);

    // Textures are in world space, so convert everything to view space
    vec4 viewPos = texture(positionTexture, texCoord);
    vec4 viewPosTo = viewPos;
    vec3 viewVec = normalize(viewPos.xyz);
    vec3 normal = normalize(texture(normalTexture, texCoord).xyz);
    vec3 refl = reflect(viewVec, normal);

    // reflection ray start and end points
    vec4 startView = vec4(viewPos.xyz, 1);
    vec4 endView = vec4(viewPos.xyz + (refl * maxDistance), 1);

    vec4 startFrag = startView;
    startFrag = projection * startFrag;
    startFrag.xyz /= startFrag.w;
    startFrag.xy = startFrag.xy * 0.5f + 0.5f;
    startFrag.xy *= texSize;

    vec4 endFrag = endView;
    endFrag = projection * endFrag;
    endFrag.xyz /= endFrag.w;
    endFrag.xy = endFrag.xy * 0.5f + 0.5f;
    endFrag.xy *= texSize;

    vec2 frag = startFrag.xy;
    uv.xy = frag / texSize;
    
    vec2 delta = endFrag.xy - startFrag.xy;

    // utilize the larger difference of the 2 axes
    float useX = abs(delta.x) >= abs(delta.y) ? 1 : 0;
    float maxDelta = mix(abs(delta.y), abs(delta.x), useX) * clamp(resolution, 0, 1);

    // determine how much to increment per march; if resolution < 1 frags will be skipped
    vec2 incr = delta / max(maxDelta, 0.001f);

    // search0 remembers the last position on the line where the ray didn't hit anything.
    // used to refine the point at which the ray actually hits later on.
    float search0 = 0, search1 = 0;
    int firstHit = 0, secondHit = 0;

    float viewDist = startView.x;
    // view distance difference between current ray point and scene position
    float depth = thickness;

    // first pass
    for (int i = 0; i < int(maxDelta); i++)
    {
        frag += incr;
        uv.xy = frag / texSize;
        viewPosTo = texture(positionTexture, uv.xy);
        
        // search1 = percentage along line that frag is currently at
        search1 = mix((frag.y - startFrag.y) / delta.y, (frag.x - startFrag.x) / delta.x, useX);
        search1 = clamp(search1, 0, 1);

        viewDist = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
        depth = viewDist - viewPosTo.y;

        if (depth > 0 && depth < thickness) // hit
        {
            firstHit = 1;
            break;
        }
        else // not hit
        {  
            search0 = search1;
        }
    }

    // set search1 to halfway between last miss and last hit
    search1 = search0 + ((search1 - search0) / 2.f);
    
    // if ray hit, do second pass
    for (int i = 0; i < steps * firstHit; i++)
    {
        frag = mix(startFrag.xy, endFrag.xy, search1);

        uv.xy = frag / texSize;
        viewPosTo = texture(positionTexture, uv.xy);

        viewDist = (startView.y * endView.y) / mix(endView.y, startView.y, search1);
        depth = viewDist - viewPosTo.y;

        if (depth > 0 && depth < thickness) // hit
        {
            secondHit = 1;
            search1 = search0 + ((search1 - search0) / 2.f);
        }
        else // no hit
        {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2.f);
            search0 = temp;
        }
    }

    // calculate visibility of reflection
    // interpolate visibility based on how much refl points toward camera
    float visibility = secondHit * viewPosTo.w * (1.f - max(dot(-viewVec, refl), 0.f));
    visibility *= 1 - clamp(depth / thickness, 0, 1); // fade out reflection further it is from exact intersection pt
    visibility *= 1 - clamp(length(viewPosTo - viewPos) / maxDistance, 0, 1); // fade out based on refl distance from start
    visibility *= uv.x < 0 || uv.x > 1 ? 0 : 1; // 0 if refl travels outside cam frustum
    visibility *= uv.y < 0 || uv.y > 1 ? 0 : 1;
    visibility = clamp(visibility, 0, 1);

    uv.zw = vec2(visibility);
    fragColor = uv;
}
