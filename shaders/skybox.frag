#version 450

layout(binding = 0) uniform SkyboxUBO {
    vec4 cameraForward;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 viewParams; // x = tanHalfFov, y = aspect
} sky;

layout(location = 0) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

// Calibrated against the current ACES pass to keep the sky present without lifting the frame.
const vec3 horizonColor = vec3(0.56, 0.48, 0.36);
const vec3 zenithColor = vec3(0.12, 0.28, 0.56);
const vec3 nadirColor = vec3(0.05, 0.05, 0.05);

void main() {
    vec2 ndc = fragUv * 2.0 - 1.0;

    float tanHalfFov = sky.viewParams.x;
    float aspect = sky.viewParams.y;

    vec3 forward = sky.cameraForward.xyz;
    vec3 right = sky.cameraRight.xyz;
    vec3 up = sky.cameraUp.xyz;

    // Flip ndc.y because Vulkan screen y is downward but world up should be at top of screen.
    vec3 dir = normalize(forward
                         + ndc.x * aspect * tanHalfFov * right
                         - ndc.y * tanHalfFov * up);

    vec3 sky_color;
    if (dir.y >= 0.0) {
        float t = smoothstep(0.0, 0.6, dir.y);
        sky_color = mix(horizonColor, zenithColor, t);
    } else {
        float t = smoothstep(0.0, 0.6, -dir.y);
        sky_color = mix(horizonColor, nadirColor, t);
    }

    outColor = vec4(sky_color, 1.0);
}
