#pragma once

#include <cstddef>
#include <optional>

namespace fire_engine
{

class AnimationState
{
public:
    AnimationState() = default;
    ~AnimationState() = default;

    AnimationState(const AnimationState&) = default;
    AnimationState& operator=(const AnimationState&) = default;
    AnimationState(AnimationState&&) noexcept = default;
    AnimationState& operator=(AnimationState&&) noexcept = default;

    [[nodiscard]] std::optional<size_t> activeAnimation() const noexcept
    {
        return activeAnimation_;
    }
    void activeAnimation(size_t index) noexcept
    {
        activeAnimation_ = index;
    }

    [[nodiscard]] bool hasActiveAnimation() const noexcept
    {
        return activeAnimation_.has_value();
    }

private:
    std::optional<size_t> activeAnimation_{};
};

} // namespace fire_engine
