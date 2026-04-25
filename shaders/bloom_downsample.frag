#version 450

// Call of Duty / Sledgehammer 13-tap weighted downsample. Sample positions
// are arranged so that the central 4×4 cluster carries 50% of the total
// weight while the outer corners carry the rest — gives a smooth low-pass
// without the "boxy" feel of a plain bilinear downsample.

layout(binding = 0) uniform sampler2D inputMip;

layout(push_constant) uniform BloomPushConstants {
    vec2 invInputResolution;
    int isFirstPass;
    int _pad0;
} pc;

layout(location = 0) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

vec3 sampleAt(vec2 uv, vec2 offset)
{
    return texture(inputMip, uv + offset * pc.invInputResolution).rgb;
}

// Karis-average for the very first pass. A single brilliant pixel can blow
// out a kernel; weighting by 1/(1+lumaAvg) caps its contribution and stops
// it from creating a square halo across the bloom chain.
vec3 karisAverage(vec3 a, vec3 b, vec3 c, vec3 d)
{
    float la = dot(a, vec3(0.2126, 0.7152, 0.0722));
    float lb = dot(b, vec3(0.2126, 0.7152, 0.0722));
    float lc = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float ld = dot(d, vec3(0.2126, 0.7152, 0.0722));
    float wa = 1.0 / (1.0 + la);
    float wb = 1.0 / (1.0 + lb);
    float wc = 1.0 / (1.0 + lc);
    float wd = 1.0 / (1.0 + ld);
    return (a * wa + b * wb + c * wc + d * wd) / (wa + wb + wc + wd);
}

void main()
{
    // 13 samples in a 4×4 + central-4 pattern (CoD layout).
    vec3 a = sampleAt(fragUv, vec2(-2.0, -2.0));
    vec3 b = sampleAt(fragUv, vec2( 0.0, -2.0));
    vec3 c = sampleAt(fragUv, vec2( 2.0, -2.0));
    vec3 d = sampleAt(fragUv, vec2(-2.0,  0.0));
    vec3 e = sampleAt(fragUv, vec2( 0.0,  0.0));
    vec3 f = sampleAt(fragUv, vec2( 2.0,  0.0));
    vec3 g = sampleAt(fragUv, vec2(-2.0,  2.0));
    vec3 h = sampleAt(fragUv, vec2( 0.0,  2.0));
    vec3 i = sampleAt(fragUv, vec2( 2.0,  2.0));
    vec3 j = sampleAt(fragUv, vec2(-1.0, -1.0));
    vec3 k = sampleAt(fragUv, vec2( 1.0, -1.0));
    vec3 l = sampleAt(fragUv, vec2(-1.0,  1.0));
    vec3 m = sampleAt(fragUv, vec2( 1.0,  1.0));

    if (pc.isFirstPass == 1) {
        // Karis-averaged groups suppress fireflies on the HDR target.
        vec3 g0 = karisAverage(j, k, l, m) * 0.5;
        vec3 g1 = karisAverage(a, b, d, e) * 0.125;
        vec3 g2 = karisAverage(b, c, e, f) * 0.125;
        vec3 g3 = karisAverage(d, e, g, h) * 0.125;
        vec3 g4 = karisAverage(e, f, h, i) * 0.125;
        outColor = vec4(g0 + g1 + g2 + g3 + g4, 1.0);
    } else {
        // Standard CoD weights.
        vec3 result = e * 0.125;
        result += (a + c + g + i) * 0.03125;
        result += (b + d + f + h) * 0.0625;
        result += (j + k + l + m) * 0.125;
        outColor = vec4(result, 1.0);
    }
}
