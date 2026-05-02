#include <fire_engine/scene/controllable.hpp>

namespace fire_engine
{

void Controllable::update(const ControllerState& state, Transform& transform,
                          const Mat4& parentWorld) const
{
    auto position = transform.position();
    position.x(position.x() + state.deltaPosition().x() * speed_);
    transform.position(position);
    transform.update(parentWorld);
}

} // namespace fire_engine
