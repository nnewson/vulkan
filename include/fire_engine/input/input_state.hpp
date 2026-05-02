#pragma once

#include <fire_engine/input/animation_state.hpp>
#include <fire_engine/input/camera_state.hpp>
#include <fire_engine/input/controller_state.hpp>

namespace fire_engine
{

class InputState
{
public:
    InputState() = default;
    ~InputState() = default;

    InputState(const InputState&) = default;
    InputState& operator=(const InputState&) = default;
    InputState(InputState&&) noexcept = default;
    InputState& operator=(InputState&&) noexcept = default;

    [[nodiscard]] const CameraState& cameraState() const noexcept
    {
        return cameraState_;
    }
    [[nodiscard]] CameraState& cameraState() noexcept
    {
        return cameraState_;
    }
    void cameraState(CameraState cs) noexcept
    {
        cameraState_ = cs;
    }

    [[nodiscard]] const AnimationState& animationState() const noexcept
    {
        return animationState_;
    }
    [[nodiscard]] AnimationState& animationState() noexcept
    {
        return animationState_;
    }
    void animationState(AnimationState as) noexcept
    {
        animationState_ = as;
    }

    [[nodiscard]] const ControllerState& controllerState() const noexcept
    {
        return controllerState_;
    }
    [[nodiscard]] ControllerState& controllerState() noexcept
    {
        return controllerState_;
    }
    void controllerState(ControllerState cs) noexcept
    {
        controllerState_ = cs;
    }

    [[nodiscard]] double time() const noexcept
    {
        return cameraState_.time();
    }
    void time(double t) noexcept
    {
        cameraState_.time(t);
        controllerState_.time(t);
    }

private:
    CameraState cameraState_{};
    AnimationState animationState_{};
    ControllerState controllerState_{};
};

} // namespace fire_engine
