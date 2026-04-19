#version 450

layout(binding = 0) uniform ShadowUBO {
    mat4 model;
    mat4 lightViewProj;
    int hasSkin;
} shadow;

layout(binding = 1) uniform SkinUBO {
    mat4 joints[64];
} skin;

layout(binding = 2) uniform MorphUBO {
    int hasMorph;
    int morphTargetCount;
    int vertexCount;
    int _pad0;
    vec4 weights[2];
} morph;

layout(std430, binding = 3) readonly buffer MorphTargets {
    vec4 data[];
} morphTargets;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uvec4 inJoints;
layout(location = 5) in vec4 inWeights;
layout(location = 6) in vec4 inTangent;

void main() {
    vec3 pos = inPos;

    if (morph.hasMorph == 1) {
        int nTargets = morph.morphTargetCount;
        int nVerts = morph.vertexCount;
        for (int i = 0; i < nTargets; i++) {
            int posOffset = i * nVerts + gl_VertexIndex;
            float w = morph.weights[i / 4][i % 4];
            pos += w * morphTargets.data[posOffset].xyz;
        }
    }

    mat4 transform;
    if (shadow.hasSkin == 1) {
        transform = inWeights.x * skin.joints[inJoints.x]
                  + inWeights.y * skin.joints[inJoints.y]
                  + inWeights.z * skin.joints[inJoints.z]
                  + inWeights.w * skin.joints[inJoints.w];
    } else {
        transform = shadow.model;
    }

    vec4 worldPos = transform * vec4(pos, 1.0);
    gl_Position = shadow.lightViewProj * worldPos;
}
