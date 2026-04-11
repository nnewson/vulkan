#pragma once

#include <fire_engine/graphics/object.hpp>
#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Mesh : public Component
{
public:
    explicit Mesh(Object object);
    ~Mesh() override = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    Object object_;
};

} // namespace fire_engine
