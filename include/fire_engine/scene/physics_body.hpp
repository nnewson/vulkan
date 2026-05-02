#pragma once

#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class PhysicsBody
{
public:
    PhysicsBody() = default;
    ~PhysicsBody() = default;

    PhysicsBody(const PhysicsBody&) = default;
    PhysicsBody& operator=(const PhysicsBody&) = default;
    PhysicsBody(PhysicsBody&&) noexcept = default;
    PhysicsBody& operator=(PhysicsBody&&) noexcept = default;

    [[nodiscard]] Vec3 velocity() const noexcept
    {
        return velocity_;
    }
    void velocity(Vec3 velocity) noexcept
    {
        velocity_ = velocity;
    }

    void reflect(Vec3 normal) noexcept;

private:
    Vec3 velocity_{};
};

} // namespace fire_engine
