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
    alignas(16) float diffuseAlpha[4]{};
    alignas(16) float emissiveRoughness[4]{};
    alignas(16) float materialParams[4]{};
    alignas(16) int textureFlags[4]{};
    alignas(16) int extraFlags[4]{};
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

struct EnvironmentCaptureUBO
{
    alignas(4) int faceIndex{0};
    alignas(4) int faceExtent{0};
    int _pad1{0};
    int _pad2{0};
};

struct LightUBO
{
    alignas(16) float direction[4]{}; // xyz normalised, w unused
    alignas(16) float colour[4]{};    // rgb, a = intensity
    alignas(16) Mat4 lightViewProj{};
    alignas(16) float iblParams[4]{};         // x = maxReflectionLod, y/z = IBL strengths
    alignas(16) float shadowParams[4]{};      // x = minBias, y = slopeBias, z = filterRadius
    alignas(16) float environmentParams[4]{}; // x = skyboxIntensity
};

struct EnvironmentPrefilterPushConstants
{
    alignas(4) int faceIndex{0};
    alignas(4) int faceExtent{0};
    float roughness{0.0f};
    float _pad0{0.0f};
};

struct ShadowUBO
{
    alignas(16) Mat4 model;
    alignas(16) Mat4 lightViewProj;
    alignas(4) int hasSkin{0};
};

} // namespace fire_engine
