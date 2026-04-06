#pragma once

#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Animator : public Component
{
public:
    Animator() = default;
    ~Animator() override = default;

    Animator(const Animator&) = default;
    Animator& operator=(const Animator&) = default;
    Animator(Animator&&) noexcept = default;
    Animator& operator=(Animator&&) noexcept = default;

    void update(const CameraState& /*input_state*/, const Transform& /*transform*/) override
    {
    }
};

} // namespace fire_engine
