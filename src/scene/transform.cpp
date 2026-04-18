#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

void Transform::update(const Mat4& parentWorld)
{
    local_ = Mat4::translate(position_) * rotation_.toMat4() * Mat4::scale(scale_);

    world_ = parentWorld * local_;
}

} // namespace fire_engine
