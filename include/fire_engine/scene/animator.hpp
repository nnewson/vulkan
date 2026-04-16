#pragma once

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

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

    [[nodiscard]]
    Mat4 modelMatrix() const noexcept override
    {
        return modelMatrix_;
    }

    void animation(Animation* anim) noexcept
    {
        animation_ = anim;
    }

    [[nodiscard]] Animation* animation() noexcept
    {
        return animation_;
    }

    [[nodiscard]] const Animation* animation() const noexcept
    {
        return animation_;
    }

private:
    Animation* animation_{nullptr};
    Mat4 modelMatrix_{Mat4::identity()};
    double startTime_{0.0};
    bool initialized_{false};
};

} // namespace fire_engine
