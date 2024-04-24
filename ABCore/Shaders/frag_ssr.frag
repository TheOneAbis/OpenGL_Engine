#version 450 core

// Texture samplers
uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D colorTexture;

// INPUTS, UNIFORM, OUTPUTS
uniform mat4 projection;
uniform vec3 cameraPosition;
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

    vec4 worldPos = texture(positionTexture, texCoord);
    vec3 viewVec = normalize(worldPos.xyz - cameraPosition);
    vec3 normal = normalize(texture(normalTexture, texCoord).xyz);
    vec3 refl = reflect(viewVec, normal);

    // reflection ray start and end points
    vec4 start = vec4(worldPos.xyz, 1);
    vec4 end = vec4(worldPos.xyz + (refl * maxDistance), 1);


}
