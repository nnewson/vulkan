#pragma once

#include <cstdint>

namespace fire_engine
{

class Joints4
{
public:
    constexpr Joints4(uint32_t j0 = 0, uint32_t j1 = 0, uint32_t j2 = 0,
                      uint32_t j3 = 0) noexcept
        : j0_(j0),
          j1_(j1),
          j2_(j2),
          j3_(j3)
    {
    }

    ~Joints4() = default;

    Joints4(const Joints4&) = default;
    Joints4& operator=(const Joints4&) = default;
    Joints4(Joints4&&) noexcept = default;
    Joints4& operator=(Joints4&&) noexcept = default;

    [[nodiscard]] constexpr uint32_t j0() const noexcept
    {
        return j0_;
    }

    void j0(uint32_t v) noexcept
    {
        j0_ = v;
    }

    [[nodiscard]] constexpr uint32_t j1() const noexcept
    {
        return j1_;
    }

    void j1(uint32_t v) noexcept
    {
        j1_ = v;
    }

    [[nodiscard]] constexpr uint32_t j2() const noexcept
    {
        return j2_;
    }

    void j2(uint32_t v) noexcept
    {
        j2_ = v;
    }

    [[nodiscard]] constexpr uint32_t j3() const noexcept
    {
        return j3_;
    }

    void j3(uint32_t v) noexcept
    {
        j3_ = v;
    }

    [[nodiscard]] constexpr bool operator==(const Joints4& other) const noexcept
    {
        return j0_ == other.j0_ && j1_ == other.j1_ && j2_ == other.j2_ && j3_ == other.j3_;
    }

private:
    uint32_t j0_{0};
    uint32_t j1_{0};
    uint32_t j2_{0};
    uint32_t j3_{0};
};

} // namespace fire_engine
