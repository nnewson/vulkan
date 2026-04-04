#pragma once

#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

struct Mat4
{
    float m[16];
};

Mat4 mat4Identity();
Mat4 mat4Mul(const Mat4& a, const Mat4& b);
Mat4 mat4RotateY(float rad);
Mat4 mat4RotateX(float rad);
Mat4 mat4LookAt(Vec3 eye, Vec3 center, Vec3 up);
Mat4 mat4Perspective(float fovRad, float aspect, float near, float far);

} // namespace fire_engine
