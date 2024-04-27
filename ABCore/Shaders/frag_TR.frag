#version 450 core

uniform sampler2D viewportTexture;

uniform float LAvg;
uniform float LMax;
uniform float LdMax;
uniform float KeyValue;
uniform bool Ward;

out vec4 fragColor;

float WardScale()
{
    float num = 1.219f + pow(LdMax / 2.f, 0.4f);
    float denom = 1.219f + pow(LAvg, 0.4f);
    return pow(num / denom, 2.5f);
}

vec3 ReinhardScale(vec3 color)
{
    vec3 scaled = (color * KeyValue) / LAvg;
    return scaled / (1.f + scaled);
}

void main()
{
    vec3 color = texture(viewportTexture, gl_FragCoord.xy / textureSize(viewportTexture, 0).xy).xyz;
    color *= LMax;

    // apply corresponding operator
    color = Ward ? color * WardScale() / LdMax : ReinhardScale(color);

    // apply device model
    fragColor = vec4(color, 1);
}
