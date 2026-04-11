#pragma once

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Mesh : public Component
{
public:
    Mesh() = default;
    explicit Mesh(Geometry geometry);
    ~Mesh() override = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    Geometry geometry_;
};

} // namespace fire_engine
