#pragma once

#include <cmath>
#include <istream>

namespace fire_engine
{

class Vec3
{
public:
    constexpr Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept
        : x_(x),
          y_(y),
          z_(z)
    {
    }

    [[nodiscard]]
    constexpr float x() const noexcept
    {
        return x_;
    }

    constexpr void x(float x) noexcept
    {
        x_ = x;
    }

    [[nodiscard]]
    constexpr float y() const noexcept
    {
        return y_;
    }

    constexpr void y(float y) noexcept
    {
        y_ = y;
    }

    [[nodiscard]]
    constexpr float z() const noexcept
    {
        return z_;
    }

    constexpr void z(float z) noexcept
    {
        z_ = z;
    }

    [[nodiscard]]
    constexpr Vec3 operator-(const Vec3& rhs) const noexcept
    {
        Vec3 result{*this};
        result -= rhs;
        return result;
    }

    constexpr Vec3& operator-=(const Vec3& rhs) noexcept
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    [[nodiscard]]
    constexpr Vec3 operator+(const Vec3& rhs) const noexcept
    {
        Vec3 result{*this};
        result += rhs;
        return result;
    }

    constexpr Vec3& operator+=(const Vec3& rhs) noexcept
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    [[nodiscard]]
    constexpr Vec3 operator*(const float rhs) const noexcept
    {
        Vec3 result{*this};
        result *= rhs;
        return result;
    }

    constexpr Vec3& operator*=(const float rhs) noexcept
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    [[nodiscard]]
    constexpr Vec3 operator/(const float rhs) const noexcept
    {
        Vec3 result{*this};
        result /= rhs;
        return result;
    }

    constexpr Vec3& operator/=(const float rhs) noexcept
    {
        x_ /= rhs;
        y_ /= rhs;
        z_ /= rhs;
        return *this;
    }

    [[nodiscard]]
    static constexpr float dotProduct(const Vec3& lhs, const Vec3& rhs) noexcept
    {
        return lhs.x_ * rhs.x_ + lhs.y_ * rhs.y_ + lhs.z_ * rhs.z_;
    }

    [[nodiscard]]
    constexpr float dotProduct(const Vec3& rhs) const noexcept
    {
        return Vec3::dotProduct(*this, rhs);
    }

    [[nodiscard]]
    static constexpr Vec3 crossProduct(const Vec3& lhs, const Vec3& rhs) noexcept
    {
        return {lhs.y_ * rhs.z_ - lhs.z_ * rhs.y_, lhs.z_ * rhs.x_ - lhs.x_ * rhs.z_,
                lhs.x_ * rhs.y_ - lhs.y_ * rhs.x_};
    }

    [[nodiscard]]
    constexpr Vec3 crossProduct(const Vec3& rhs) const noexcept
    {
        return Vec3::crossProduct(*this, rhs);
    }

    [[nodiscard]] constexpr float magnitude() const noexcept
    {
        return std::sqrt(magnitudeSquared());
    }

    [[nodiscard]] constexpr float magnitudeSquared() const noexcept
    {
        return x_ * x_ + y_ * y_ + z_ * z_;
    }

    [[nodiscard]]
    static constexpr Vec3 normalise(const Vec3& v) noexcept
    {
        float len = v.magnitude();
        if (len < 1e-8f)
        {
            return {0.0f, 0.0f, 0.0f};
        }

        return {v.x_ / len, v.y_ / len, v.z_ / len};
    }

    constexpr Vec3& normalise() noexcept
    {
        *this = normalise(*this);
        return *this;
    }

    [[nodiscard]]
    constexpr bool operator==(const Vec3& rhs) const noexcept
    {
        return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_;
    }

    // Friend declaration
    friend std::istream& operator>>(std::istream& is, Vec3& v)
    {
        return is >> v.x_ >> v.y_ >> v.z_;
    }

private:
    float x_;
    float y_;
    float z_;
};

} // namespace fire_engine
