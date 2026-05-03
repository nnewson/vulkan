#pragma once

#include <cstdint>
#include <variant>

#include <fire_engine/collision/collider.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/physics/physics_body.hpp>

namespace fire_engine
{

struct AabbShape
{
    AABB bounds{};
};

struct BoxShape
{
    Vec3 halfExtents{0.5f, 0.5f, 0.5f};
    Vec3 center{};
};

struct SphereShape
{
    float radius{0.5f};
    Vec3 center{};
};

struct CapsuleShape
{
    float radius{0.5f};
    float halfHeight{0.5f};
    Vec3 center{};
};

using ColliderShape = std::variant<AabbShape, BoxShape, SphereShape, CapsuleShape>;

struct ColliderDesc
{
    ColliderShape shape{AabbShape{}};
    std::uint32_t collisionLayer{1U};
    std::uint32_t collisionMask{~0U};
    PhysicsMaterial material{};
};

} // namespace fire_engine
