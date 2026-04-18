#include <fire_engine/scene/empty.hpp>

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
