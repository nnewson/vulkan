#include <fire_engine/animation/linear_animation.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace fire_engine
{

namespace
{

float loopTime(float t, float dur) noexcept
{
    if (dur <= 0.0f)
    {
        return 0.0f;
    }
    t = std::fmod(t, dur);
    if (t < 0.0f)
    {
        t += dur;
    }
    return t;
}

// Find the bracketing index and interpolation alpha in the keyframe list.
// Returns (i, alpha) where keyframes[i].time <= t < keyframes[i+1].time and
// alpha in [0, 1]. Clamps at both endpoints: if t is before the first keyframe
// returns (0, 0); if t is past the last keyframe returns (last, 0). This matches
// the glTF spec, which says pre-first and post-last samples take the boundary
// key's value.
template <typename KF>
std::pair<std::size_t, float> findBracket(const std::vector<KF>& kf, float t) noexcept
{
    // Clamp to the first keyframe if t is before it (glTF spec: pre-first values
    // use the first key's value). Without this, the loop below returns (0, alpha<0)
    // and the caller extrapolates backwards off the curve.
    if (t <= kf.front().time)
    {
        return {0, 0.0f};
    }

    std::size_t i = 0;
    for (; i < kf.size() - 1; ++i)
    {
        if (t < kf[i + 1].time)
        {
            break;
        }
    }
    if (i >= kf.size() - 1)
    {
        return {kf.size() - 1, 0.0f};
    }

    float dt = kf[i + 1].time - kf[i].time;
    float alpha = (dt > 0.0f) ? (t - kf[i].time) / dt : 0.0f;
    return {i, alpha};
}

Quaternion sampleRotation(const std::vector<LinearAnimation::RotationKeyframe>& kf,
                          float t) noexcept
{
    if (kf.empty())
    {
        return Quaternion::identity();
    }
    if (kf.size() == 1)
    {
        return {kf[0].qx, kf[0].qy, kf[0].qz, kf[0].qw};
    }

    auto [i, alpha] = findBracket(kf, t);
    Quaternion q0{kf[i].qx, kf[i].qy, kf[i].qz, kf[i].qw};
    if (alpha == 0.0f)
    {
        return q0;
    }
    Quaternion q1{kf[i + 1].qx, kf[i + 1].qy, kf[i + 1].qz, kf[i + 1].qw};
    return Quaternion::slerp(q0, q1, alpha);
}

Vec3 sampleTranslation(const std::vector<LinearAnimation::TranslationKeyframe>& kf,
                       float t) noexcept
{
    if (kf.empty())
    {
        return {};
    }
    if (kf.size() == 1)
    {
        return kf[0].position;
    }

    auto [i, alpha] = findBracket(kf, t);
    if (alpha == 0.0f)
    {
        return kf[i].position;
    }
    return kf[i].position + (kf[i + 1].position - kf[i].position) * alpha;
}

} // namespace

float LinearAnimation::duration() const noexcept
{
    if (duration_ >= 0.0f)
    {
        return duration_;
    }
    float rotDur = rotationKeyframes_.empty() ? 0.0f : rotationKeyframes_.back().time;
    float transDur = translationKeyframes_.empty() ? 0.0f : translationKeyframes_.back().time;
    return std::max(rotDur, transDur);
}

Mat4 LinearAnimation::sample(float t) const noexcept
{
    if (rotationKeyframes_.empty() && translationKeyframes_.empty())
    {
        return Mat4::identity();
    }

    float looped = loopTime(t, duration());

    Quaternion rotation = sampleRotation(rotationKeyframes_, looped);
    Vec3 position = sampleTranslation(translationKeyframes_, looped);

    return Mat4::translate(position) * rotation.toMat4();
}

} // namespace fire_engine
