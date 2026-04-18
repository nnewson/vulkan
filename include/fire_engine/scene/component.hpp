#pragma once

#include <fire_engine/input/input_state.hpp>
#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

struct RenderContext;

class Component
{
public:
    Component() = default;
    virtual ~Component() = default;

    Component(const Component&) = default;
    Component& operator=(const Component&) = default;
    Component(Component&&) noexcept = default;
    Component& operator=(Component&&) noexcept = default;

    virtual void update(const InputState& input_state, const Transform& transform) = 0;

    [[nodiscard]]
    virtual Mat4 render(const RenderContext& ctx, const Mat4& world) = 0;

    [[nodiscard]]
    virtual Mat4 modelMatrix() const noexcept
    {
        return Mat4::identity();
    }
};

} // namespace fire_engine
