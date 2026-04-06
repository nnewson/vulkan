#include "fire_engine/input/camera_state.hpp"
#include <fire_engine/scene/node.hpp>

namespace fire_engine
{

void Camera::update(const CameraState& input_state, const Transform& transform)
{
    position_ += input_state.deltaPosition();
    yaw_ += input_state.deltaYaw();
    pitch_ += input_state.deltaPitch();
}

} // namespace fire_engine
