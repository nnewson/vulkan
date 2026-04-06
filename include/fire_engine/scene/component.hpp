#pragma once

#include <fire_engine/input/camera_state.hpp>
#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

class Component
{
public:
    Component() = default;
    virtual ~Component() = default;

    Component(const Component&) = default;
    Component& operator=(const Component&) = default;
    Component(Component&&) noexcept = default;
    Component& operator=(Component&&) noexcept = default;

    virtual void update(const CameraState& input_state, const Transform& transform) = 0;
};

} // namespace fire_engine
