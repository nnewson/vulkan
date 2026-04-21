#version 450

layout(binding = 0) uniform sampler2D hdrInput;

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
    vec3 mapped = acesApproximation(hdr);
    vec3 gamma = pow(mapped, vec3(1.0 / 2.2));
    outColor = vec4(gamma, 1.0);
}
