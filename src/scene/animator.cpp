#include <fire_engine/scene/animator.hpp>

#include <fire_engine/core/system.hpp>

namespace fire_engine
{

void Animator::update(const CameraState& /*input_state*/, const Transform& /*transform*/)
{
    if (!initialized_)
    {
        startTime_ = System::getTime();
        initialized_ = true;
    }

    float t = static_cast<float>(System::getTime() - startTime_);
    modelMatrix_ = animation_.sample(t);
}

Mat4 Animator::render(const RenderContext& /*ctx*/, const Mat4& world)
{
    return world * modelMatrix_;
}

} // namespace fire_engine
