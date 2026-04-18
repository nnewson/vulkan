#include <fire_engine/scene/animator.hpp>

#include <fire_engine/animation/animation.hpp>

namespace fire_engine
{

void Animator::addAnimation(Animation* anim)
{
    animations_.push_back(anim);
}

void Animator::activeAnimation(std::size_t index) noexcept
{
    if (index < animations_.size())
    {
        activeIndex_ = index;
        initialized_ = false;
    }
}

void Animator::update(const InputState& input_state, const Transform& /*transform*/)
{
    if (animations_.empty())
    {
        return;
    }

    const auto& animState = input_state.animationState();
    if (animState.hasActiveAnimation())
    {
        auto index = *animState.activeAnimation();
        if (index < animations_.size() && index != activeIndex_)
        {
            activeAnimation(index);
        }
    }

    if (!initialized_)
    {
        startTime_ = input_state.time();
        initialized_ = true;
    }

    float t = static_cast<float>(input_state.time() - startTime_);
    modelMatrix_ = animations_[activeIndex_]->sample(t);
}

Mat4 Animator::render(const RenderContext& /*ctx*/, const Mat4& world)
{
    return world * modelMatrix_;
}

} // namespace fire_engine
