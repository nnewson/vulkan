#pragma once

#include <cstdint>

namespace fire_engine
{

class ColliderId
{
public:
    static constexpr std::uint32_t invalidValue = 0;

    constexpr ColliderId() noexcept = default;
    constexpr explicit ColliderId(std::uint32_t value) noexcept
        : value_{value}
    {
    }

    ~ColliderId() = default;

    ColliderId(const ColliderId&) = default;
    ColliderId& operator=(const ColliderId&) = default;
    ColliderId(ColliderId&&) noexcept = default;
    ColliderId& operator=(ColliderId&&) noexcept = default;

    [[nodiscard]]
    constexpr std::uint32_t value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
        return value_ != invalidValue;
    }

    [[nodiscard]]
    friend constexpr bool operator==(ColliderId lhs, ColliderId rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    [[nodiscard]]
    friend constexpr bool operator!=(ColliderId lhs, ColliderId rhs) noexcept
    {
        return !(lhs == rhs);
    }

    [[nodiscard]]
    friend constexpr bool operator<(ColliderId lhs, ColliderId rhs) noexcept
    {
        return lhs.value_ < rhs.value_;
    }

private:
    std::uint32_t value_{invalidValue};
};

} // namespace fire_engine
