#version 450

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragColor = inColor;
    fragNormal = mat3(ubo.model) * inNormal;
    fragWorldPos = worldPos.xyz;
    fragTexCoord = inTexCoord;
}
