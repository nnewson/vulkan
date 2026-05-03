#pragma once

#include <vector>

#include <fire_engine/math/vec3.hpp>
#include <fire_engine/physics/physics_handle.hpp>

namespace fire_engine
{

struct Contact
{
    PhysicsBodyHandle firstBody;
    PhysicsBodyHandle secondBody;
    PhysicsColliderHandle firstCollider;
    PhysicsColliderHandle secondCollider;
    float toi{0.0f};
    Vec3 normal{};
};

struct ContactManifold
{
    PhysicsBodyHandle firstBody;
    PhysicsBodyHandle secondBody;
    std::vector<Contact> contacts;
};

} // namespace fire_engine
