#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

void Transform::update(const Mat4& parentWorld)
{
    local_ = Mat4::translate(position_) * Mat4::rotateY(rotation_.y()) *
             Mat4::rotateX(rotation_.x()) * Mat4::rotateZ(rotation_.z()) * Mat4::scale(scale_);

    world_ = parentWorld * local_;
}

} // namespace fire_engine
