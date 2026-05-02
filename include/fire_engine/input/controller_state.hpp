#pragma once

#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class ControllerState
{
public:
    ControllerState() = default;
    ~ControllerState() = default;

    ControllerState(const ControllerState&) = default;
    ControllerState& operator=(const ControllerState&) = default;
    ControllerState(ControllerState&&) noexcept = default;
    ControllerState& operator=(ControllerState&&) noexcept = default;

    [[nodiscard]] Vec3 deltaPosition() const noexcept
    {
        return deltaPosition_;
    }
    void deltaPosition(Vec3 dp) noexcept
    {
        deltaPosition_ = dp;
    }

    [[nodiscard]] double time() const noexcept
    {
        return time_;
    }
    void time(double t) noexcept
    {
        time_ = t;
    }

private:
    Vec3 deltaPosition_{};
    double time_{0.0};
};

} // namespace fire_engine
