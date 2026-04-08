#pragma once

#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

struct UniformBufferObject
{
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    alignas(16) float cameraPos[4];
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
    float _pad0;
};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

} // namespace fire_engine
