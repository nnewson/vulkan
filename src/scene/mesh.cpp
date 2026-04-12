#include <fire_engine/scene/mesh.hpp>

#include <fire_engine/animation/linear_animation.hpp>
#include <fire_engine/render/render_context.hpp>

namespace fire_engine
{

Mesh::Mesh(Object object)
    : object_(std::move(object))
{
}

void Mesh::update(const CameraState& input_state, const Transform& /*transform*/)
{
    object_.updateSkin();

    if (morphAnimation_ != nullptr)
    {
        if (!morphInitialized_)
        {
            startTime_ = input_state.time();
            morphInitialized_ = true;
        }

        float t = static_cast<float>(input_state.time() - startTime_);
        morphWeights_ = morphAnimation_->sampleWeights(t, morphWeights_.size());
    }

    if (!morphWeights_.empty())
    {
        object_.morphWeights(morphWeights_);
    }
}

Mat4 Mesh::render(const RenderContext& ctx, const Mat4& world)
{
    return object_.render(ctx, world);
}

} // namespace fire_engine
