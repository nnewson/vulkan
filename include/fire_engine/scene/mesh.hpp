#pragma once

#include <vector>

#include <fire_engine/graphics/object.hpp>
#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class LinearAnimation;
class Skin;

class Mesh : public Component
{
public:
    explicit Mesh(Object object);
    ~Mesh() override = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void skin(Skin* s) noexcept
    {
        object_.skin(s);
    }

    void morphAnimation(LinearAnimation* anim) noexcept
    {
        morphAnimation_ = anim;
    }

    void initialMorphWeights(std::vector<float> w) noexcept
    {
        morphWeights_ = std::move(w);
    }

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    Object object_;
    LinearAnimation* morphAnimation_{nullptr};
    std::vector<float> morphWeights_;
    double startTime_{0.0};
    bool morphInitialized_{false};
};

} // namespace fire_engine
