#include <fire_engine/input/input.hpp>

namespace fire_engine
{

InputState Input::update(const Window& window, float deltaTime)
{
    window.pollEvents();

    keyboard_.poll(window);
    mouse_.poll(window);

    // WASD movement (camera-relative: z = forward/back, x = strafe) plus
    // E/F world-space vertical movement on Y.
    Vec3 delta{};

    if (keyboard_.w())
    {
        delta.z(delta.z() + speed_ * deltaTime);
    }
    if (keyboard_.s())
    {
        delta.z(delta.z() - speed_ * deltaTime);
    }
    if (keyboard_.a())
    {
        delta.x(delta.x() + speed_ * deltaTime);
    }
    if (keyboard_.d())
    {
        delta.x(delta.x() - speed_ * deltaTime);
    }
    if (keyboard_.e())
    {
        delta.y(delta.y() + speed_ * deltaTime);
    }
    if (keyboard_.f())
    {
        delta.y(delta.y() - speed_ * deltaTime);
    }

    // Left mouse button drag = camera-relative movement (same as WASD)
    if (mouse_.leftButton())
    {
        delta.x(delta.x() + static_cast<float>(mouse_.deltaX()) * panSensitivity_);
        delta.z(delta.z() - static_cast<float>(mouse_.deltaY()) * panSensitivity_);
    }

    CameraState cameraState;
    cameraState.deltaPosition(delta);

    ControllerState controllerState;
    Vec3 controllerDelta{};
    if (keyboard_.left())
    {
        controllerDelta.x(controllerDelta.x() - deltaTime);
    }
    if (keyboard_.right())
    {
        controllerDelta.x(controllerDelta.x() + deltaTime);
    }
    controllerState.deltaPosition(controllerDelta);

    // Right mouse button drag = rotation (yaw/pitch)
    if (mouse_.rightButton())
    {
        cameraState.deltaYaw(static_cast<float>(mouse_.deltaX()) * sensitivity_);
        cameraState.deltaPitch(-static_cast<float>(mouse_.deltaY()) * sensitivity_);
    }

    // Scroll wheel = zoom
    double scroll = mouse_.consumeScrollDelta();
    if (scroll != 0.0)
    {
        cameraState.deltaZoom(static_cast<float>(scroll) * zoomSpeed_);
    }

    InputState state;
    state.deltaTime(deltaTime);
    state.cameraState(cameraState);
    state.controllerState(controllerState);

    if (keyboard_.one())
    {
        state.animationState().activeAnimation(0);
    }
    else if (keyboard_.two())
    {
        state.animationState().activeAnimation(1);
    }
    else if (keyboard_.three())
    {
        state.animationState().activeAnimation(2);
    }

    return state;
}

void Input::enable(const Window& window)
{
    mouse_.registerScrollCallback(window);
}

} // namespace fire_engine
