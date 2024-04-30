#version 450 core

const uint WARD = 0u;
const uint REINHARD = 1u;
const uint ALM = 2u;

uniform sampler2D viewportTexture;

uniform float LAvg;
uniform float LMax;
uniform float LdMax;
uniform float bias;
uniform float KeyValue;
uniform uint operator;

out vec4 fragColor;

float ToLuminance(vec3 color)
{
    return 0.27f * color.r + 0.67f * color.g + 0.06f * color.b;
}

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

float AdaptiveLog(vec3 color)
{
    float lw = ToLuminance(color) / LAvg;

    float ld = 0;
    float LwMax = LMax / LAvg;
    float part1 = 1.f / (log(LwMax + 1.f) / log(10.f));
    float part2 = log(lw + 1) / log(2.f + pow(lw / LwMax, log(bias) / log(0.5f)) * 8.f);

    return part1 * part2;
}

void main()
{
    vec3 color = texture(viewportTexture, gl_FragCoord.xy / textureSize(viewportTexture, 0).xy).xyz;
    color *= LMax;

    // apply corresponding operator
    switch (operator)
    {
        case WARD:
            color = color * WardScale() / LdMax;
        break;

        case REINHARD:
            color = ReinhardScale(color);
        break;

        case ALM:
            color = color * AdaptiveLog(color) / LdMax;
        break;
    }

    // apply device model
    fragColor = vec4(color, 1);
}
