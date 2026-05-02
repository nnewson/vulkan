#pragma once

#include <fire_engine/input/input_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

struct RenderContext;

class Empty
{
public:
    Empty() = default;
    ~Empty() = default;

    Empty(const Empty&) = default;
    Empty& operator=(const Empty&) = default;
    Empty(Empty&&) noexcept = default;
    Empty& operator=(Empty&&) noexcept = default;

    void update(const InputState& input_state, const Transform& transform);

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world);
};

} // namespace fire_engine
