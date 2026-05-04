#pragma once

#include <cstdint>

namespace fire_engine
{

class PhysicsBodyHandle
{
public:
    constexpr PhysicsBodyHandle() noexcept = default;
    constexpr explicit PhysicsBodyHandle(std::uint32_t value) noexcept
        : value_{value}
    {
    }

    ~PhysicsBodyHandle() = default;

    PhysicsBodyHandle(const PhysicsBodyHandle&) = default;
    PhysicsBodyHandle& operator=(const PhysicsBodyHandle&) = default;
    PhysicsBodyHandle(PhysicsBodyHandle&&) noexcept = default;
    PhysicsBodyHandle& operator=(PhysicsBodyHandle&&) noexcept = default;

    [[nodiscard]]
    constexpr std::uint32_t value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
        return value_ != 0U;
    }

    [[nodiscard]]
    friend constexpr bool operator==(PhysicsBodyHandle lhs, PhysicsBodyHandle rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    [[nodiscard]]
    friend constexpr bool operator!=(PhysicsBodyHandle lhs, PhysicsBodyHandle rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:
    std::uint32_t value_{0U};
};

class PhysicsColliderHandle
{
public:
    constexpr PhysicsColliderHandle() noexcept = default;
    constexpr explicit PhysicsColliderHandle(std::uint32_t value) noexcept
        : value_{value}
    {
    }

    ~PhysicsColliderHandle() = default;

    PhysicsColliderHandle(const PhysicsColliderHandle&) = default;
    PhysicsColliderHandle& operator=(const PhysicsColliderHandle&) = default;
    PhysicsColliderHandle(PhysicsColliderHandle&&) noexcept = default;
    PhysicsColliderHandle& operator=(PhysicsColliderHandle&&) noexcept = default;

    [[nodiscard]]
    constexpr std::uint32_t value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
        return value_ != 0U;
    }

    [[nodiscard]]
    friend constexpr bool operator==(PhysicsColliderHandle lhs, PhysicsColliderHandle rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    [[nodiscard]]
    friend constexpr bool operator!=(PhysicsColliderHandle lhs, PhysicsColliderHandle rhs) noexcept
    {
        return !(lhs == rhs);
    }

private:
    std::uint32_t value_{0U};
};

class PhysicsConstraintHandle
{
public:
    constexpr PhysicsConstraintHandle() noexcept = default;
    constexpr explicit PhysicsConstraintHandle(std::uint32_t value) noexcept
        : value_{value}
    {
    }

    ~PhysicsConstraintHandle() = default;

    PhysicsConstraintHandle(const PhysicsConstraintHandle&) = default;
    PhysicsConstraintHandle& operator=(const PhysicsConstraintHandle&) = default;
    PhysicsConstraintHandle(PhysicsConstraintHandle&&) noexcept = default;
    PhysicsConstraintHandle& operator=(PhysicsConstraintHandle&&) noexcept = default;

    [[nodiscard]]
    constexpr std::uint32_t value() const noexcept
    {
        return value_;
    }

    [[nodiscard]]
    constexpr bool valid() const noexcept
    {
        return value_ != 0U;
    }

private:
    std::uint32_t value_{0U};
};

} // namespace fire_engine
