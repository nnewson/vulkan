#pragma once

#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Transform
{
public:
    Transform() = default;
    ~Transform() = default;

    Transform(const Transform&) = default;
    Transform& operator=(const Transform&) = default;
    Transform(Transform&&) noexcept = default;
    Transform& operator=(Transform&&) noexcept = default;

    [[nodiscard]] Vec3 position() const noexcept
    {
        return position_;
    }
    void position(Vec3 pos) noexcept
    {
        position_ = pos;
    }

    [[nodiscard]] Vec3 rotation() const noexcept
    {
        return rotation_;
    }
    void rotation(Vec3 rot) noexcept
    {
        rotation_ = rot;
    }

    [[nodiscard]] Vec3 scale() const noexcept
    {
        return scale_;
    }
    void scale(Vec3 s) noexcept
    {
        scale_ = s;
    }

    [[nodiscard]] Mat4 local() const noexcept
    {
        return local_;
    }
    [[nodiscard]] Mat4 world() const noexcept
    {
        return world_;
    }

    void update(const Mat4& parentWorld);

private:
    Vec3 position_{};
    Vec3 rotation_{};
    Vec3 scale_{1.0f, 1.0f, 1.0f};
    Mat4 local_{Mat4::identity()};
    Mat4 world_{Mat4::identity()};
};

} // namespace fire_engine
