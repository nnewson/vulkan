#include <fire_engine/scene/mesh.hpp>

#include <fire_engine/animation/animation.hpp>
#include <fire_engine/render/render_context.hpp>

namespace fire_engine
{

Mesh::Mesh(Object object)
    : object_(std::move(object))
{
}

void Mesh::addMorphAnimation(Animation* anim)
{
    morphAnimations_.push_back(anim);
}

void Mesh::activeMorphAnimation(std::size_t index) noexcept
{
    if (index < morphAnimations_.size())
    {
        activeMorphIndex_ = index;
        morphInitialized_ = false;
    }
}

void Mesh::update(const InputState& input_state, const Transform& /*transform*/)
{
    object_.updateSkin();

    const auto& animState = input_state.animationState();
    if (animState.hasActiveAnimation())
    {
        auto index = *animState.activeAnimation();
        if (index < morphAnimations_.size() && index != activeMorphIndex_)
        {
            activeMorphAnimation(index);
        }
    }

    if (!morphAnimations_.empty())
    {
        if (!morphInitialized_)
        {
            startTime_ = input_state.time();
            morphInitialized_ = true;
        }

        float t = static_cast<float>(input_state.time() - startTime_);
        morphWeights_ =
            morphAnimations_[activeMorphIndex_]->sampleWeights(t, morphWeights_.size());
    }

    if (!morphWeights_.empty())
    {
        object_.morphWeights(morphWeights_);
    }
}

Mat4 Mesh::render(const RenderContext& ctx, const Mat4& world)
{
    auto commands = object_.render(ctx.frameInfo(), world);
    if (ctx.drawCommands != nullptr)
    {
        ctx.drawCommands->insert(ctx.drawCommands->end(), commands.begin(), commands.end());
    }
    return world;
}

} // namespace fire_engine
