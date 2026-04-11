#include <fire_engine/scene/mesh.hpp>

#include <fire_engine/renderer/render_context.hpp>

namespace fire_engine
{

Mesh::Mesh(Geometry geometry)
    : geometry_(std::move(geometry))
{
}

void Mesh::update(const CameraState& /*input_state*/, const Transform& /*transform*/)
{
}

Mat4 Mesh::render(const RenderContext& ctx, const Mat4& world)
{
    return geometry_.render(ctx, world);
}

} // namespace fire_engine
