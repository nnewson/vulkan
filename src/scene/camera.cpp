#include <fire_engine/scene/node.hpp>

namespace fire_engine
{

void Camera::update(const CameraState& input_state, const Transform& transform)
{
    localPosition_ += input_state.deltaPosition();
    localYaw_ += input_state.deltaYaw();
    localPitch_ = clampPitch(localPitch_ + input_state.deltaPitch());

    Vec3 tp = transform.position();
    Vec3 tr = transform.rotation();

    worldPosition_ = localPosition_ + tp;
    worldYaw_ = localYaw_ + tr.y();
    worldPitch_ = clampPitch(localPitch_ + tr.x());
}

} // namespace fire_engine
