#pragma once

#include <vector>

#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

class LinearAnimation
{
public:
    struct Keyframe
    {
        float time{0.0f};
        float qx{0.0f};
        float qy{0.0f};
        float qz{0.0f};
        float qw{1.0f};
    };

    LinearAnimation() = default;
    ~LinearAnimation() = default;

    LinearAnimation(const LinearAnimation&) = default;
    LinearAnimation& operator=(const LinearAnimation&) = default;
    LinearAnimation(LinearAnimation&&) noexcept = default;
    LinearAnimation& operator=(LinearAnimation&&) noexcept = default;

    [[nodiscard]] const std::vector<Keyframe>& keyframes() const noexcept
    {
        return keyframes_;
    }
    void keyframes(std::vector<Keyframe> kf) noexcept
    {
        keyframes_ = std::move(kf);
    }

    [[nodiscard]] float duration() const noexcept
    {
        return keyframes_.empty() ? 0.0f : keyframes_.back().time;
    }

    [[nodiscard]] Mat4 sample(float t) const noexcept;

private:
    struct Quat
    {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float w{1.0f};
    };

    [[nodiscard]] static Quat slerp(Quat a, Quat b, float t) noexcept;
    [[nodiscard]] static Mat4 quatToMat4(Quat q) noexcept;

    std::vector<Keyframe> keyframes_;
};

} // namespace fire_engine
