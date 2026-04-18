#pragma once

#include <cstddef>
#include <vector>

#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Animation;

class Animator : public Component
{
public:
    Animator() = default;
    ~Animator() override = default;

    Animator(const Animator&) = default;
    Animator& operator=(const Animator&) = default;
    Animator(Animator&&) noexcept = default;
    Animator& operator=(Animator&&) noexcept = default;

    void update(const InputState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

    [[nodiscard]]
    Mat4 modelMatrix() const noexcept override
    {
        return modelMatrix_;
    }

    void addAnimation(Animation* anim);

    void activeAnimation(std::size_t index) noexcept;

    [[nodiscard]] std::size_t activeAnimation() const noexcept
    {
        return activeIndex_;
    }

    [[nodiscard]] std::size_t animationCount() const noexcept
    {
        return animations_.size();
    }

    [[nodiscard]] Animation* animation() noexcept
    {
        if (animations_.empty())
        {
            return nullptr;
        }
        return animations_[activeIndex_];
    }

    [[nodiscard]] const Animation* animation() const noexcept
    {
        if (animations_.empty())
        {
            return nullptr;
        }
        return animations_[activeIndex_];
    }

private:
    std::vector<Animation*> animations_;
    std::size_t activeIndex_{0};
    Mat4 modelMatrix_{Mat4::identity()};
    double startTime_{0.0};
    bool initialized_{false};
};

} // namespace fire_engine
