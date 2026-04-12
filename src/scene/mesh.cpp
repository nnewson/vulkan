#include <fire_engine/scene/mesh.hpp>

#include <fire_engine/render/render_context.hpp>

namespace fire_engine
{

Mesh::Mesh(Object object)
    : object_(std::move(object))
{
}

void Mesh::update(const CameraState& /*input_state*/, const Transform& /*transform*/)
{
    object_.updateSkin();
}

Mat4 Mesh::render(const RenderContext& ctx, const Mat4& world)
{
    return object_.render(ctx, world);
}

} // namespace fire_engine
