#pragma once

#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Empty : public Component
{
public:
    Empty() = default;
    ~Empty() override = default;

    Empty(const Empty&) = default;
    Empty& operator=(const Empty&) = default;
    Empty(Empty&&) noexcept = default;
    Empty& operator=(Empty&&) noexcept = default;

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;
};

} // namespace fire_engine
