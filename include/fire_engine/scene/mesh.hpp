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
    struct MorphAnimationEntry
    {
        std::size_t id{0};
        Animation* animation{nullptr};
    };

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

    void addMorphAnimation(std::size_t id, Animation* anim);
    void addMorphAnimation(Animation* anim)
    {
        addMorphAnimation(morphAnimations_.size(), anim);
    }

    void activeMorphAnimation(std::size_t id) noexcept;

    [[nodiscard]] std::size_t activeMorphAnimation() const noexcept
    {
        return activeMorphAnimationId_;
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
        return morphAnimations_[activeMorphIndex_].animation;
    }

    [[nodiscard]] const Animation* morphAnimation() const noexcept
    {
        if (morphAnimations_.empty())
        {
            return nullptr;
        }
        return morphAnimations_[activeMorphIndex_].animation;
    }

    void initialMorphWeights(std::vector<float> w) noexcept
    {
        morphWeights_ = std::move(w);
    }

    void update(const InputState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    [[nodiscard]] std::size_t findMorphAnimationIndex(std::size_t id) const noexcept;

    Object object_;
    std::vector<MorphAnimationEntry> morphAnimations_;
    std::size_t activeMorphIndex_{0};
    std::size_t activeMorphAnimationId_{0};
    std::vector<float> morphWeights_;
    double startTime_{0.0};
    bool morphInitialized_{false};
};

} // namespace fire_engine
