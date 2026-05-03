#pragma once

#include <cstddef>
#include <limits>

#include <fire_engine/collision/collider_id.hpp>

namespace fire_engine
{

class Collider;
class SweepAndPruneBroadPhase;

enum class CollisionAxis
{
    X,
    Y,
    Z
};

class EndPoint
{
public:
    static constexpr std::size_t invalidIndex = std::numeric_limits<std::size_t>::max();

    EndPoint() = default;
    constexpr EndPoint(Collider* collider, CollisionAxis axis, bool isMin) noexcept
        : collider_{collider},
          axis_{axis},
          isMin_{isMin}
    {
    }

    ~EndPoint() = default;

    EndPoint(const EndPoint&) = delete;
    EndPoint& operator=(const EndPoint&) = delete;
    EndPoint(EndPoint&&) noexcept = default;
    EndPoint& operator=(EndPoint&&) noexcept = default;

    [[nodiscard]]
    constexpr float value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr ColliderId colliderId() const noexcept
    {
        return colliderId_;
    }

    [[nodiscard]]
    constexpr CollisionAxis axis() const noexcept
    {
        return axis_;
    }

    [[nodiscard]]
    constexpr std::size_t index() const noexcept
    {
        return index_;
    }

    [[nodiscard]]
    constexpr bool isMin() const noexcept
    {
        return isMin_;
    }

private:
    friend class Collider;
    friend class SweepAndPruneBroadPhase;

    float value_{0.0f};
    Collider* collider_{nullptr};
    ColliderId colliderId_;
    CollisionAxis axis_{CollisionAxis::X};
    std::size_t index_{invalidIndex};
    bool isMin_{true};

    constexpr void value(float value) noexcept
    {
        value_ = value;
    }

    [[nodiscard]]
    constexpr Collider* collider() const noexcept
    {
        return collider_;
    }

    constexpr void collider(Collider* collider) noexcept
    {
        collider_ = collider;
    }

    constexpr void colliderId(ColliderId colliderId) noexcept
    {
        colliderId_ = colliderId;
    }

    constexpr void axis(CollisionAxis axis) noexcept
    {
        axis_ = axis;
    }

    constexpr void index(std::size_t index) noexcept
    {
        index_ = index;
    }

    constexpr void isMin(bool isMin) noexcept
    {
        isMin_ = isMin;
    }
};

} // namespace fire_engine
