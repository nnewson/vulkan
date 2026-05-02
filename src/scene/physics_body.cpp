#include <fire_engine/scene/physics_body.hpp>

namespace fire_engine
{

void PhysicsBody::reflect(Vec3 normal) noexcept
{
    velocity_ -= normal * (2.0f * Vec3::dotProduct(velocity_, normal));
}

} // namespace fire_engine
