#pragma once

#include <cstddef>
#include <vector>

#include <fire_engine/graphics/object.hpp>
#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Animation;
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

    void addMorphAnimation(Animation* anim);

    void activeMorphAnimation(std::size_t index) noexcept;

    [[nodiscard]] std::size_t activeMorphAnimation() const noexcept
    {
        return activeMorphIndex_;
    }

    [[nodiscard]] std::size_t morphAnimationCount() const noexcept
    {
        return morphAnimations_.size();
    }

    [[nodiscard]] Animation* morphAnimation() noexcept
    {
        if (morphAnimations_.empty())
        {
            return nullptr;
        }
        return morphAnimations_[activeMorphIndex_];
    }

    [[nodiscard]] const Animation* morphAnimation() const noexcept
    {
        if (morphAnimations_.empty())
        {
            return nullptr;
        }
        return morphAnimations_[activeMorphIndex_];
    }

    void initialMorphWeights(std::vector<float> w) noexcept
    {
        morphWeights_ = std::move(w);
    }

    void update(const InputState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    Object object_;
    std::vector<Animation*> morphAnimations_;
    std::size_t activeMorphIndex_{0};
    std::vector<float> morphWeights_;
    double startTime_{0.0};
    bool morphInitialized_{false};
};

} // namespace fire_engine
