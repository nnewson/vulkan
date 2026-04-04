#pragma once

#include <cmath>

#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Mat4
{
public:
    constexpr Mat4() noexcept
        : m_{}
    {
    }

    [[nodiscard]]
    constexpr float operator()(int row, int col) const noexcept
    {
        return m_[col * 4 + row];
    }

    constexpr void set(int row, int col, float value) noexcept
    {
        m_[col * 4 + row] = value;
    }

    [[nodiscard]]
    constexpr const float* data() const noexcept
    {
        return m_;
    }

    [[nodiscard]]
    static constexpr Mat4 identity() noexcept
    {
        Mat4 r;
        r.m_[0] = r.m_[5] = r.m_[10] = r.m_[15] = 1.0f;
        return r;
    }

    [[nodiscard]]
    constexpr Mat4 operator*(const Mat4& rhs) const noexcept
    {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    r.m_[c * 4 + row] += m_[k * 4 + row] * rhs.m_[c * 4 + k];
        return r;
    }

    constexpr Mat4& operator*=(const Mat4& rhs) noexcept
    {
        *this = *this * rhs;
        return *this;
    }

    [[nodiscard]]
    constexpr bool operator==(const Mat4& rhs) const noexcept
    {
        for (int i = 0; i < 16; ++i)
            if (m_[i] != rhs.m_[i])
                return false;
        return true;
    }

    [[nodiscard]]
    static Mat4 rotateY(float rad) noexcept
    {
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m_[0] = c;
        r.m_[8] = s;
        r.m_[2] = -s;
        r.m_[10] = c;
        return r;
    }

    [[nodiscard]]
    static Mat4 rotateX(float rad) noexcept
    {
        Mat4 r = identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m_[5] = c;
        r.m_[9] = -s;
        r.m_[6] = s;
        r.m_[10] = c;
        return r;
    }

    [[nodiscard]]
    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) noexcept
    {
        float fx = center.x() - eye.x(), fy = center.y() - eye.y(), fz = center.z() - eye.z();
        float len = std::sqrt(fx * fx + fy * fy + fz * fz);
        fx /= len;
        fy /= len;
        fz /= len;
        float sx = fy * up.z() - fz * up.y(), sy = fz * up.x() - fx * up.z(),
              sz = fx * up.y() - fy * up.x();
        len = std::sqrt(sx * sx + sy * sy + sz * sz);
        sx /= len;
        sy /= len;
        sz /= len;
        float ux = sy * fz - sz * fy, uy = sz * fx - sx * fz, uz = sx * fy - sy * fx;
        Mat4 r = identity();
        r.m_[0] = sx;
        r.m_[4] = sy;
        r.m_[8] = sz;
        r.m_[12] = -(sx * eye.x() + sy * eye.y() + sz * eye.z());
        r.m_[1] = ux;
        r.m_[5] = uy;
        r.m_[9] = uz;
        r.m_[13] = -(ux * eye.x() + uy * eye.y() + uz * eye.z());
        r.m_[2] = -fx;
        r.m_[6] = -fy;
        r.m_[10] = -fz;
        r.m_[14] = (fx * eye.x() + fy * eye.y() + fz * eye.z());
        return r;
    }

    [[nodiscard]]
    static Mat4 perspective(float fovRad, float aspect, float near, float far) noexcept
    {
        float tanHalf = std::tan(fovRad / 2.0f);
        Mat4 r;
        r.m_[0] = 1.0f / (aspect * tanHalf);
        r.m_[5] = -1.0f / tanHalf; // Vulkan Y is flipped vs OpenGL
        r.m_[10] = far / (near - far);
        r.m_[11] = -1.0f;
        r.m_[14] = (near * far) / (near - far);
        return r;
    }

private:
    float m_[16];
};

} // namespace fire_engine
