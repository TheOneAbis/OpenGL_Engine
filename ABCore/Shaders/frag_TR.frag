#version 450 core

uniform sampler2D viewportTexture;

uniform vec3 avgIntensity;

out vec4 fragColor;

void main()
{
    vec2 texSize = textureSize(viewportTexture, 0).xy;
    vec2 texCoord = gl_FragCoord.xy / texSize;
    fragColor = texture(viewportTexture, texCoord);
}
