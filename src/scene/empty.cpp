#include <fire_engine/scene/empty.hpp>

#include <fire_engine/input/input_state.hpp>
#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

void Empty::update(const InputState& /*input_state*/, const Transform& /*transform*/)
{
}

Mat4 Empty::render(const RenderContext& /*ctx*/, const Mat4& world)
{
    return world;
}

} // namespace fire_engine
