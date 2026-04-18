#include <fire_engine/scene/node.hpp>

namespace fire_engine
{

void Camera::update(const InputState& input_state, const Transform& transform)
{
    const auto& cs = input_state.cameraState();

    // Apply rotation first (from right-mouse drag)
    localYaw_ += cs.deltaYaw();
    localPitch_ = clampPitch(localPitch_ + cs.deltaPitch());

    // Compute camera basis vectors from yaw
    float cosYaw = std::cos(localYaw_);
    float sinYaw = std::sin(localYaw_);
    Vec3 forwardXZ{cosYaw, 0.0f, sinYaw};
    Vec3 right{sinYaw, 0.0f, -cosYaw};

    // WASD and left-mouse drag movement (camera-relative XZ plane)
    const Vec3& delta = cs.deltaPosition();
    localPosition_ = localPosition_ + forwardXZ * delta.z() + right * delta.x();

    // Scroll wheel zoom (along full 3D view direction)
    float cosPitch = std::cos(localPitch_);
    float sinPitch = std::sin(localPitch_);
    Vec3 forward3D{cosPitch * cosYaw, sinPitch, cosPitch * sinYaw};
    localPosition_ = localPosition_ + forward3D * cs.deltaZoom();

    Vec3 tp = transform.position();
    Vec3 tr = transform.rotation().toEulerXYZ();

    worldPosition_ = localPosition_ + tp;
    worldYaw_ = localYaw_ + tr.y();
    worldPitch_ = clampPitch(localPitch_ + tr.x());
}

Mat4 Camera::render(const RenderContext& /*ctx*/, const Mat4& world)
{
    return world;
}

} // namespace fire_engine
