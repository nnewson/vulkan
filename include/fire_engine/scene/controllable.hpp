#pragma once

#include <fire_engine/input/controller_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

class Controllable
{
public:
    Controllable() = default;
    ~Controllable() = default;

    Controllable(const Controllable&) = default;
    Controllable& operator=(const Controllable&) = default;
    Controllable(Controllable&&) noexcept = default;
    Controllable& operator=(Controllable&&) noexcept = default;

    void update(const ControllerState& state, Transform& transform, const Mat4& parentWorld) const;

private:
    static constexpr float speed_{10.0f};
};

} // namespace fire_engine
