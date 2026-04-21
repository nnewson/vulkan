#include <fire_engine/scene/mesh.hpp>

#include <fire_engine/animation/animation.hpp>
#include <fire_engine/animation/animation_selection.hpp>
#include <fire_engine/render/render_context.hpp>

namespace fire_engine
{

Mesh::Mesh(Object object)
    : object_(std::move(object))
{
}

std::size_t Mesh::findMorphAnimationIndex(std::size_t id) const noexcept
{
    return findAnimationEntryIndex(morphAnimations_, id);
}

void Mesh::addMorphAnimation(std::size_t id, Animation* anim)
{
    morphAnimations_.push_back({id, anim});
    if (morphAnimations_.size() == 1)
    {
        activeMorphIndex_ = 0;
        activeMorphAnimationId_ = id;
    }
}

void Mesh::activeMorphAnimation(std::size_t id) noexcept
{
    (void)selectAnimationEntry(morphAnimations_, id, activeMorphIndex_, activeMorphAnimationId_,
                               morphInitialized_);
}

void Mesh::update(const InputState& input_state, const Transform& /*transform*/)
{
    object_.updateSkin();

    const auto& animState = input_state.animationState();
    if (animState.hasActiveAnimation())
    {
        auto id = *animState.activeAnimation();
        auto index = findMorphAnimationIndex(id);
        if (index < morphAnimations_.size() && index != activeMorphIndex_)
        {
            (void)selectAnimationEntry(morphAnimations_, id, activeMorphIndex_,
                                       activeMorphAnimationId_, morphInitialized_);
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
            morphAnimations_[activeMorphIndex_].animation->sampleWeights(t, morphWeights_.size());
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
