#version 450 core

uniform sampler2D viewportTexture;

uniform float LAvg;
uniform float LMax;
uniform float LdMax;
uniform float KeyValue;
uniform bool Ward;

out vec4 fragColor;

float ToLuminance(vec3 color)
{
    color *= LMax;
    return 0.27f * color.r + 0.67f * color.g + 0.06f * color.b;
}

float WardScale(float l)
{
    float num = 1.219f + pow(LdMax / 2.f, 0.4f);
    float denom = 1.219f + pow(LAvg, 0.4f);
    return pow(num / denom, 2.5f);
}

vec3 ReinhardScale(vec3 color)
{
    return color / (1.f + color * (KeyValue / LAvg));
}

void main()
{
    vec3 color = texture(viewportTexture, gl_FragCoord.xy / textureSize(viewportTexture, 0).xy).xyz;

    // calculate color luminance scaled to [0, LMax]
    float l = ToLuminance(color);

    // apply corresponding operator
    color =  Ward ? color * WardScale(l) : ReinhardScale(color);

    // apply device model
    fragColor = vec4(color / LdMax, 1);
}
