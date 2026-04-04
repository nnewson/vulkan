#include <fire_engine/math.hpp>

#include <cmath>

namespace fire_engine
{

Mat4 mat4Identity()
{
    Mat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

Mat4 mat4Mul(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.m[c * 4 + row] += a.m[k * 4 + row] * b.m[c * 4 + k];
    return r;
}

Mat4 mat4RotateY(float rad)
{
    Mat4 r = mat4Identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0] = c;
    r.m[8] = s;
    r.m[2] = -s;
    r.m[10] = c;
    return r;
}

Mat4 mat4RotateX(float rad)
{
    Mat4 r = mat4Identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[5] = c;
    r.m[9] = -s;
    r.m[6] = s;
    r.m[10] = c;
    return r;
}

Mat4 mat4LookAt(Vec3 eye, Vec3 center, Vec3 up)
{
    float fx = center.x_ - eye.x_, fy = center.y_ - eye.y_, fz = center.z_ - eye.z_;
    float len = sqrtf(fx * fx + fy * fy + fz * fz);
    fx /= len;
    fy /= len;
    fz /= len;
    float sx = fy * up.z_ - fz * up.y_, sy = fz * up.x_ - fx * up.z_, sz = fx * up.y_ - fy * up.x_;
    len = sqrtf(sx * sx + sy * sy + sz * sz);
    sx /= len;
    sy /= len;
    sz /= len;
    float ux = sy * fz - sz * fy, uy = sz * fx - sx * fz, uz = sx * fy - sy * fx;
    Mat4 r = mat4Identity();
    r.m[0] = sx;
    r.m[4] = sy;
    r.m[8] = sz;
    r.m[12] = -(sx * eye.x_ + sy * eye.y_ + sz * eye.z_);
    r.m[1] = ux;
    r.m[5] = uy;
    r.m[9] = uz;
    r.m[13] = -(ux * eye.x_ + uy * eye.y_ + uz * eye.z_);
    r.m[2] = -fx;
    r.m[6] = -fy;
    r.m[10] = -fz;
    r.m[14] = (fx * eye.x_ + fy * eye.y_ + fz * eye.z_);
    return r;
}

Mat4 mat4Perspective(float fovRad, float aspect, float near, float far)
{
    float tanHalf = tanf(fovRad / 2.0f);
    Mat4 r{};
    r.m[0] = 1.0f / (aspect * tanHalf);
    r.m[5] = -1.0f / tanHalf; // Vulkan Y is flipped vs OpenGL
    r.m[10] = far / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (near * far) / (near - far);
    return r;
}

} // namespace fire_engine
