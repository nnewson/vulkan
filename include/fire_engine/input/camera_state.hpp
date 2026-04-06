#pragma once

#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class CameraState
{
public:
    CameraState() = default;
    ~CameraState() = default;

    CameraState(const CameraState&) = default;
    CameraState& operator=(const CameraState&) = default;
    CameraState(CameraState&&) noexcept = default;
    CameraState& operator=(CameraState&&) noexcept = default;

    [[nodiscard]] Vec3 deltaPosition() const noexcept
    {
        return deltaPosition_;
    }
    void deltaPosition(Vec3 dp) noexcept
    {
        deltaPosition_ = dp;
    }

    [[nodiscard]] float deltaPitch() const noexcept
    {
        return deltaPitch_;
    }
    void deltaPitch(float dp) noexcept
    {
        deltaPitch_ = dp;
    }

    [[nodiscard]] float deltaYaw() const noexcept
    {
        return deltaYaw_;
    }
    void deltaYaw(float dy) noexcept
    {
        deltaYaw_ = dy;
    }

private:
    Vec3 deltaPosition_{};
    float deltaPitch_{0.0f};
    float deltaYaw_{0.0f};
};

} // namespace fire_engine
