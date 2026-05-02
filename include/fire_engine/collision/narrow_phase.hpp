#pragma once

#include <optional>

#include <fire_engine/collision/collider.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

struct SweptAabbContact
{
    float toi{0.0f};
    Vec3 normal{};
};

class NarrowPhase
{
public:
    NarrowPhase() = default;
    ~NarrowPhase() = default;

    NarrowPhase(const NarrowPhase&) = default;
    NarrowPhase& operator=(const NarrowPhase&) = default;
    NarrowPhase(NarrowPhase&&) noexcept = default;
    NarrowPhase& operator=(NarrowPhase&&) noexcept = default;

    [[nodiscard]]
    std::optional<SweptAabbContact> sweptAabb(const Collider& moving,
                                              const Collider& target) const noexcept;
};

} // namespace fire_engine
