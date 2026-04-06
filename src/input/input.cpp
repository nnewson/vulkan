#include <fire_engine/input/input.hpp>

namespace fire_engine
{

CameraState Input::update(const Window& window, float deltaTime)
{
    keyboard_.poll(window);

    if (keyboard_.escape())
    {
        mouse_.release(window);
    }
    mouse_.capture(window);

    Vec3 delta{};

    if (keyboard_.w())
        delta.z(delta.z() - speed_ * deltaTime);
    if (keyboard_.s())
        delta.z(delta.z() + speed_ * deltaTime);
    if (keyboard_.a())
        delta.x(delta.x() - speed_ * deltaTime);
    if (keyboard_.d())
        delta.x(delta.x() + speed_ * deltaTime);
    if (keyboard_.q())
        delta.y(delta.y() + speed_ * deltaTime);
    if (keyboard_.e())
        delta.y(delta.y() - speed_ * deltaTime);

    CameraState state;
    state.deltaPosition(delta);

    mouse_.poll(window);
    if (mouse_.captured())
    {
        state.deltaYaw(static_cast<float>(mouse_.deltaX()) * sensitivity_);
        state.deltaPitch(-static_cast<float>(mouse_.deltaY()) * sensitivity_);
    }

    return state;
}

void Input::enable(const Window& window)
{
    mouse_.enable(window);
}

} // namespace fire_engine
