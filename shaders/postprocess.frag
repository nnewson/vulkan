#version 450

layout(binding = 0) uniform sampler2D hdrInput;
layout(binding = 1) uniform sampler2D bloomInput;

layout(push_constant) uniform PostProcessPushConstants {
    float bloomStrength;
    float _pad0;
    float _pad1;
    float _pad2;
} pc;

layout(location = 0) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

vec3 acesApproximation(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(hdrInput, fragUv).rgb;
    // Bloom mip 0 carries the summed contribution from every coarser mip.
    // Mix is a lerp not an add so bloomStrength = 0 yields the original image.
    vec3 bloom = texture(bloomInput, fragUv).rgb;
    vec3 composited = mix(hdr, hdr + bloom, pc.bloomStrength);

    vec3 mapped = acesApproximation(composited);
    vec3 gamma = pow(mapped, vec3(1.0 / 2.2));
    outColor = vec4(gamma, 1.0);
}
