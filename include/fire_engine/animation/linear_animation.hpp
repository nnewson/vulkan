#pragma once

#include <vector>

#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/quaternion.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class LinearAnimation
{
public:
    struct RotationKeyframe
    {
        float time{0.0f};
        float qx{0.0f};
        float qy{0.0f};
        float qz{0.0f};
        float qw{1.0f};
    };

    struct TranslationKeyframe
    {
        float time{0.0f};
        Vec3 position{};
    };

    struct ScaleKeyframe
    {
        float time{0.0f};
        Vec3 scale{1.0f, 1.0f, 1.0f};
    };

    LinearAnimation() = default;
    ~LinearAnimation() = default;

    LinearAnimation(const LinearAnimation&) = default;
    LinearAnimation& operator=(const LinearAnimation&) = default;
    LinearAnimation(LinearAnimation&&) noexcept = default;
    LinearAnimation& operator=(LinearAnimation&&) noexcept = default;

    [[nodiscard]] const std::vector<RotationKeyframe>& rotationKeyframes() const noexcept
    {
        return rotationKeyframes_;
    }
    void rotationKeyframes(std::vector<RotationKeyframe> kf) noexcept
    {
        rotationKeyframes_ = std::move(kf);
    }

    [[nodiscard]] const std::vector<TranslationKeyframe>& translationKeyframes() const noexcept
    {
        return translationKeyframes_;
    }
    void translationKeyframes(std::vector<TranslationKeyframe> kf) noexcept
    {
        translationKeyframes_ = std::move(kf);
    }

    [[nodiscard]] const std::vector<ScaleKeyframe>& scaleKeyframes() const noexcept
    {
        return scaleKeyframes_;
    }
    void scaleKeyframes(std::vector<ScaleKeyframe> kf) noexcept
    {
        scaleKeyframes_ = std::move(kf);
    }

    [[nodiscard]] float duration() const noexcept;
    // Explicitly override the loop duration. Used by the glTF loader so every
    // LinearAnimation participating in the same glTF animation loops in lockstep
    // across its full span (max of *all* channels, not just this node's channels).
    // A negative value (the default) means "use the max keyframe time across my
    // own channels".
    void duration(float d) noexcept
    {
        duration_ = d;
    }

    [[nodiscard]] Mat4 sample(float t) const noexcept;

private:
    std::vector<RotationKeyframe> rotationKeyframes_;
    std::vector<TranslationKeyframe> translationKeyframes_;
    std::vector<ScaleKeyframe> scaleKeyframes_;
    float duration_{-1.0f};
};

} // namespace fire_engine
