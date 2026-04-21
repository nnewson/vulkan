#version 450

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    int hasSkin;
} ubo;

layout(binding = 3) uniform SkinUBO {
    mat4 joints[64];
} skin;

layout(binding = 4) uniform MorphUBO {
    int hasMorph;
    int morphTargetCount;
    int vertexCount;
    int _pad0;
    vec4 weights[2];
} morph;

// Morph target deltas: [pos0..posN, norm0..normN] packed as vec4 (w unused)
layout(std430, binding = 5) readonly buffer MorphTargets {
    vec4 data[];
} morphTargets;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uvec4 inJoints;
layout(location = 5) in vec4 inWeights;
layout(location = 6) in vec4 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out mat3 fragTBN;

void main() {
    vec3 pos = inPos;
    vec3 normal = inNormal;

    // Apply morph targets
    if (morph.hasMorph == 1) {
        int nTargets = morph.morphTargetCount;
        int nVerts = morph.vertexCount;
        for (int i = 0; i < nTargets; i++) {
            int posOffset = i * nVerts + gl_VertexIndex;
            int normOffset = (nTargets + i) * nVerts + gl_VertexIndex;
            float w = morph.weights[i / 4][i % 4];
            pos += w * morphTargets.data[posOffset].xyz;
            normal += w * morphTargets.data[normOffset].xyz;
        }
    }

    mat4 transform;
    if (ubo.hasSkin == 1) {
        transform = inWeights.x * skin.joints[inJoints.x]
                  + inWeights.y * skin.joints[inJoints.y]
                  + inWeights.z * skin.joints[inJoints.z]
                  + inWeights.w * skin.joints[inJoints.w];
    } else {
        transform = ubo.model;
    }

    vec4 worldPos = transform * vec4(pos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragColor = inColor;

    mat3 normalMatrix = transpose(inverse(mat3(transform)));
    fragNormal = normalMatrix * normal;
    fragWorldPos = worldPos.xyz;
    fragTexCoord = inTexCoord;

    // TBN matrix for normal mapping
    vec3 N = normalize(fragNormal);
    vec3 T = normalMatrix * inTangent.xyz;
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T)) * inTangent.w;
    fragTBN = mat3(T, B, N);
}
