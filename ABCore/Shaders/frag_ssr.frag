#version 450 core

// Texture samplers
layout (binding = 0) uniform sampler2D colorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D positionTexture;

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

    // Textures are in world space, so convert everything to view space
    vec4 viewPos = view * texture(positionTexture, texCoord);
    vec3 viewVec = normalize(viewPos.xyz);
    vec3 normal = normalize(mat3(view) * texture(normalTexture, texCoord).xyz);
    vec3 refl = reflect(viewVec, normal);

    // reflection ray start and end points
    vec4 start = vec4(viewPos.xyz, 1);
    vec4 end = vec4(viewPos.xyz + (refl * maxDistance), 1);

    vec4 startFrag = projection * start; // convert from view to screen
    startFrag.xyz /= startFrag.w;
    startFrag.xy = (startFrag.xy * 0.5f + 0.5f) * texSize; // convert screen to UV to frag coords (is this really necessary?)

    vec4 endFrag = projection * end;
    endFrag.xyz /= endFrag.w;
    endFrag.xy = (endFrag.xy * 0.5f + 0.5f) * texSize;
}
