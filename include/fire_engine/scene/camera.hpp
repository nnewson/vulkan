#pragma once

#include <cmath>

#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    Camera(const Camera&) = default;
    Camera& operator=(const Camera&) = default;
    Camera(Camera&&) noexcept = default;
    Camera& operator=(Camera&&) noexcept = default;

    [[nodiscard]]
    Vec3 position() const noexcept
    {
        return position_;
    }

    void position(Vec3 pos) noexcept
    {
        position_ = pos;
    }

    [[nodiscard]]
    float yaw() const noexcept
    {
        return yaw_;
    }

    void yaw(float yaw) noexcept
    {
        yaw_ = yaw;
    }

    [[nodiscard]]
    float pitch() const noexcept
    {
        return pitch_;
    }

    void pitch(float pitch) noexcept
    {
        pitch_ = clampPitch(pitch);
    }

    [[nodiscard]]
    Vec3 target() const noexcept
    {
        return {
            position_.x() + std::cos(pitch_) * std::cos(yaw_),
            position_.y() + std::sin(pitch_),
            position_.z() + std::cos(pitch_) * std::sin(yaw_),
        };
    }

private:
    static float clampPitch(float p) noexcept
    {
        constexpr float maxPitch = 1.5f;
        if (p > maxPitch)
            return maxPitch;
        if (p < -maxPitch)
            return -maxPitch;
        return p;
    }

    Vec3 position_{2.0f, 2.0f, 2.0f};
    float yaw_{-2.356f};
    float pitch_{-0.615f};
};

} // namespace fire_engine
