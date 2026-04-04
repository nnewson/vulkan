#pragma once

#include <cmath>

namespace fire_engine
{

struct Vec3
{
    float x_{0.0f};
    float y_{0.0f};
    float z_{0.0f};

    Vec3 operator-(const Vec3& rhs) const noexcept
    {
        return {x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_};
    }

    static Vec3 crossProduct(const Vec3& lhs, const Vec3& rhs) noexcept
    {
        return {lhs.y_ * rhs.z_ - lhs.z_ * rhs.y_, lhs.z_ * rhs.x_ - lhs.x_ * rhs.z_,
                lhs.x_ * rhs.y_ - lhs.y_ * rhs.x_};
    }

    Vec3 crossProduct(const Vec3& rhs) const noexcept
    {
        return Vec3::crossProduct(*this, rhs);
    }

    static Vec3 normalise(const Vec3& v) noexcept
    {
        float len = sqrtf(v.x_ * v.x_ + v.y_ * v.y_ + v.z_ * v.z_);
        if (len < 1e-8f)
        {
            return {0.0f, 0.0f, 0.0f};
        }

        return {v.x_ / len, v.y_ / len, v.z_ / len};
    }

    Vec3 normalise() const noexcept
    {
        return Vec3::normalise(*this);
    }

    bool operator==(const Vec3& rhs) const noexcept
    {
        return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_;
    }
};

} // namespace fire_engine
