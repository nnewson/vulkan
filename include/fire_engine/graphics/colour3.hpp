#pragma once

#include <istream>

namespace fire_engine
{

class Colour3
{
public:
    constexpr Colour3(float r = 0.0f, float g = 0.0f, float b = 0.0f) noexcept
        : r_(r),
          g_(g),
          b_(b)
    {
    }

    [[nodiscard]]
    constexpr float r() const noexcept
    {
        return r_;
    }

    constexpr void r(float r) noexcept
    {
        r_ = r;
    }

    [[nodiscard]]
    constexpr float g() const noexcept
    {
        return g_;
    }

    constexpr void g(float g) noexcept
    {
        g_ = g;
    }

    [[nodiscard]]
    constexpr float b() const noexcept
    {
        return b_;
    }

    constexpr void b(float b) noexcept
    {
        b_ = b;
    }

    [[nodiscard]]
    constexpr bool operator==(const Colour3& other) const noexcept
    {
        return r_ == other.r_ && g_ == other.g_ && b_ == other.b_;
    }

    // Friend declaration
    friend std::istream& operator>>(std::istream& is, Colour3& c)
    {
        return is >> c.r_ >> c.g_ >> c.b_;
    }

private:
    float r_;
    float g_;
    float b_;
};

} // namespace fire_engine
