#pragma once

#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Mesh : public Component
{
public:
    Mesh() = default;
    ~Mesh() override = default;

    Mesh(const Mesh&) = default;
    Mesh& operator=(const Mesh&) = default;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void update(const CameraState& /*input_state*/, const Transform& /*transform*/) override {}
};

} // namespace fire_engine
