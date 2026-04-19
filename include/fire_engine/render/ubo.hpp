#pragma once

#include <fire_engine/math/mat4.hpp>
#include <fire_engine/render/constants.hpp>

namespace fire_engine
{

struct UniformBufferObject
{
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    alignas(16) float cameraPos[4];
    alignas(4) int hasSkin{0};
    int _pad1{0};
    int _pad2{0};
    int _pad3{0};
};

struct MaterialUBO
{
    alignas(16) float ambient[3];
    alignas(16) float diffuse[3];
    alignas(16) float specular[3];
    alignas(16) float emissive[3];
    float shininess;
    float ior;
    float transparency;
    int illum;
    float roughness;
    float metallic;
    float sheen;
    float clearcoat;
    float clearcoatRoughness;
    float anisotropy;
    float anisotropyRotation;
    float alphaCutoff;
    int hasTexture{0};
    int hasEmissiveTexture{0};
    int hasNormalTexture{0};
    int hasMetallicRoughnessTexture{0};
    int hasOcclusionTexture{0};
};

struct SkinUBO
{
    Mat4 joints[MAX_JOINTS];
};

struct MorphUBO
{
    alignas(4) int hasMorph{0};
    alignas(4) int morphTargetCount{0};
    alignas(4) int vertexCount{0};
    int _pad0{0};
    float weights[MAX_MORPH_TARGETS]{};
};

struct SkyboxUBO
{
    alignas(16) float cameraForward[4]{};
    alignas(16) float cameraRight[4]{};
    alignas(16) float cameraUp[4]{};
    alignas(16) float viewParams[4]{}; // x = tanHalfFov, y = aspect
};

} // namespace fire_engine
