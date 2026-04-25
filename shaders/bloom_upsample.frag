#version 450

// 9-tap tent filter. Gathers from the coarser mip and writes the result
// additively (pipeline blend is eOne/eOne) onto the next finer mip. The
// chain effect — sum of all coarser mips back into mip 0 — is what gives
// bloom its signature halo falloff.

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

void main()
{
    // 3×3 tent kernel — corners 1/16, edges 2/16, centre 4/16. Sums to 1.
    vec3 a = sampleAt(fragUv, vec2(-1.0, -1.0));
    vec3 b = sampleAt(fragUv, vec2( 0.0, -1.0)) * 2.0;
    vec3 c = sampleAt(fragUv, vec2( 1.0, -1.0));
    vec3 d = sampleAt(fragUv, vec2(-1.0,  0.0)) * 2.0;
    vec3 e = sampleAt(fragUv, vec2( 0.0,  0.0)) * 4.0;
    vec3 f = sampleAt(fragUv, vec2( 1.0,  0.0)) * 2.0;
    vec3 g = sampleAt(fragUv, vec2(-1.0,  1.0));
    vec3 h = sampleAt(fragUv, vec2( 0.0,  1.0)) * 2.0;
    vec3 i = sampleAt(fragUv, vec2( 1.0,  1.0));

    vec3 result = (a + b + c + d + e + f + g + h + i) / 16.0;
    outColor = vec4(result, 1.0);
}
