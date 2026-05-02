#include <fire_engine/collision/narrow_phase.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <fire_engine/math/constants.hpp>

namespace fire_engine
{

namespace
{
enum class Axis
{
    X,
    Y,
    Z,
};

[[nodiscard]]
float axisMin(const AABB& bounds, Axis axis) noexcept
{
    switch (axis)
    {
    case Axis::X:
        return bounds.min.x();
    case Axis::Y:
        return bounds.min.y();
    case Axis::Z:
        return bounds.min.z();
    }

    return bounds.min.x();
}

[[nodiscard]]
float axisMax(const AABB& bounds, Axis axis) noexcept
{
    switch (axis)
    {
    case Axis::X:
        return bounds.max.x();
    case Axis::Y:
        return bounds.max.y();
    case Axis::Z:
        return bounds.max.z();
    }

    return bounds.max.x();
}

[[nodiscard]]
Vec3 axisNormal(Axis axis, float direction) noexcept
{
    switch (axis)
    {
    case Axis::X:
        return {direction, 0.0f, 0.0f};
    case Axis::Y:
        return {0.0f, direction, 0.0f};
    case Axis::Z:
        return {0.0f, 0.0f, direction};
    }

    return {direction, 0.0f, 0.0f};
}

[[nodiscard]]
bool intervalsOverlap(const AABB& lhs, const AABB& rhs, Axis axis) noexcept
{
    return axisMin(lhs, axis) <= axisMax(rhs, axis) && axisMax(lhs, axis) >= axisMin(rhs, axis);
}

[[nodiscard]]
bool aabbOverlap(const AABB& lhs, const AABB& rhs) noexcept
{
    return intervalsOverlap(lhs, rhs, Axis::X) && intervalsOverlap(lhs, rhs, Axis::Y) &&
           intervalsOverlap(lhs, rhs, Axis::Z);
}

[[nodiscard]]
float axisComponent(Vec3 value, Axis axis) noexcept
{
    switch (axis)
    {
    case Axis::X:
        return value.x();
    case Axis::Y:
        return value.y();
    case Axis::Z:
        return value.z();
    }

    return value.x();
}

[[nodiscard]]
Vec3 boundsDelta(const AABB& previous, const AABB& current) noexcept
{
    return {current.min.x() - previous.min.x(), current.min.y() - previous.min.y(),
            current.min.z() - previous.min.z()};
}

[[nodiscard]]
Vec3 startingOverlapNormal(const AABB& movingBounds, const AABB& targetBounds) noexcept
{
    const std::array<Axis, 3> axes{Axis::X, Axis::Y, Axis::Z};
    Axis bestAxis = Axis::X;
    float bestDepth = std::numeric_limits<float>::max();

    for (Axis axis : axes)
    {
        const float depth = std::min(axisMax(movingBounds, axis), axisMax(targetBounds, axis)) -
                            std::max(axisMin(movingBounds, axis), axisMin(targetBounds, axis));
        if (depth < bestDepth)
        {
            bestDepth = depth;
            bestAxis = axis;
        }
    }

    const float movingCentre =
        (axisMin(movingBounds, bestAxis) + axisMax(movingBounds, bestAxis)) * 0.5f;
    const float targetCentre =
        (axisMin(targetBounds, bestAxis) + axisMax(targetBounds, bestAxis)) * 0.5f;
    return axisNormal(bestAxis, movingCentre < targetCentre ? -1.0f : 1.0f);
}

} // namespace

std::optional<SweptAabbContact> NarrowPhase::sweptAabb(const Collider& moving,
                                                       const Collider& target) const noexcept
{
    const AABB movingStart = moving.previousWorldBounds();
    const AABB movingEnd = moving.worldBounds();
    const AABB targetStart = target.previousWorldBounds();
    const AABB targetEnd = target.worldBounds();
    const Vec3 relativeDelta =
        boundsDelta(movingStart, movingEnd) - boundsDelta(targetStart, targetEnd);

    if (relativeDelta.magnitudeSquared() < float_epsilon * float_epsilon)
    {
        if (!aabbOverlap(movingEnd, targetEnd))
        {
            return std::nullopt;
        }
        return SweptAabbContact{0.0f, startingOverlapNormal(movingEnd, targetEnd)};
    }

    const std::array<Axis, 3> axes{Axis::X, Axis::Y, Axis::Z};
    float entryTime = -std::numeric_limits<float>::infinity();
    float exitTime = std::numeric_limits<float>::infinity();
    Vec3 normal{};

    for (Axis axis : axes)
    {
        const float delta = axisComponent(relativeDelta, axis);
        float axisEntry = -std::numeric_limits<float>::infinity();
        float axisExit = std::numeric_limits<float>::infinity();
        Vec3 axisEntryNormal{};

        if (std::abs(delta) < float_epsilon)
        {
            if (!intervalsOverlap(movingStart, targetStart, axis))
            {
                return std::nullopt;
            }
        }
        else if (delta > 0.0f)
        {
            axisEntry = (axisMin(targetStart, axis) - axisMax(movingStart, axis)) / delta;
            axisExit = (axisMax(targetStart, axis) - axisMin(movingStart, axis)) / delta;
            axisEntryNormal = axisNormal(axis, -1.0f);
        }
        else
        {
            axisEntry = (axisMax(targetStart, axis) - axisMin(movingStart, axis)) / delta;
            axisExit = (axisMin(targetStart, axis) - axisMax(movingStart, axis)) / delta;
            axisEntryNormal = axisNormal(axis, 1.0f);
        }

        if (axisEntry > entryTime)
        {
            entryTime = axisEntry;
            normal = axisEntryNormal;
        }
        exitTime = std::min(exitTime, axisExit);
    }

    if (entryTime > exitTime || exitTime < 0.0f || entryTime > 1.0f)
    {
        return std::nullopt;
    }

    if (entryTime < 0.0f)
    {
        return SweptAabbContact{0.0f, startingOverlapNormal(movingStart, targetStart)};
    }

    return SweptAabbContact{entryTime, normal};
}

} // namespace fire_engine
