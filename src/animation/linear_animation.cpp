#include <fire_engine/animation/linear_animation.hpp>

#include <cmath>

namespace fire_engine
{

Mat4 LinearAnimation::sample(float t) const noexcept
{
    if (keyframes_.empty())
    {
        return Mat4::identity();
    }

    if (keyframes_.size() == 1)
    {
        return quatToMat4({keyframes_[0].qx, keyframes_[0].qy, keyframes_[0].qz, keyframes_[0].qw});
    }

    // Loop the animation
    float dur = duration();
    if (dur > 0.0f)
    {
        t = std::fmod(t, dur);
        if (t < 0.0f)
        {
            t += dur;
        }
    }

    // Find bracketing keyframes
    std::size_t i = 0;
    for (; i < keyframes_.size() - 1; ++i)
    {
        if (t < keyframes_[i + 1].time)
        {
            break;
        }
    }

    // Clamp to last keyframe
    if (i >= keyframes_.size() - 1)
    {
        const auto& kf = keyframes_.back();
        return quatToMat4({kf.qx, kf.qy, kf.qz, kf.qw});
    }

    const auto& kf0 = keyframes_[i];
    const auto& kf1 = keyframes_[i + 1];

    float dt = kf1.time - kf0.time;
    float alpha = (dt > 0.0f) ? (t - kf0.time) / dt : 0.0f;

    Quat q0{kf0.qx, kf0.qy, kf0.qz, kf0.qw};
    Quat q1{kf1.qx, kf1.qy, kf1.qz, kf1.qw};

    return quatToMat4(slerp(q0, q1, alpha));
}

LinearAnimation::Quat LinearAnimation::slerp(Quat a, Quat b, float t) noexcept
{
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    // If the dot product is negative, negate one quaternion to take the shorter path
    if (dot < 0.0f)
    {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }

    // If quaternions are very close, use linear interpolation to avoid division by zero
    if (dot > 0.9995f)
    {
        Quat result{
            a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z),
            a.w + t * (b.w - a.w),
        };
        // Normalise
        float len = std::sqrt(result.x * result.x + result.y * result.y + result.z * result.z +
                              result.w * result.w);
        if (len > 0.0f)
        {
            result.x /= len;
            result.y /= len;
            result.z /= len;
            result.w /= len;
        }
        return result;
    }

    float theta = std::acos(dot);
    float sinTheta = std::sin(theta);

    float wa = std::sin((1.0f - t) * theta) / sinTheta;
    float wb = std::sin(t * theta) / sinTheta;

    return {
        wa * a.x + wb * b.x,
        wa * a.y + wb * b.y,
        wa * a.z + wb * b.z,
        wa * a.w + wb * b.w,
    };
}

Mat4 LinearAnimation::quatToMat4(Quat q) noexcept
{
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;

    // Column-major rotation matrix from unit quaternion
    Mat4 m;
    // Column 0
    m[0, 0] = 1.0f - 2.0f * (yy + zz);
    m[1, 0] = 2.0f * (xy + wz);
    m[2, 0] = 2.0f * (xz - wy);
    // Column 1
    m[0, 1] = 2.0f * (xy - wz);
    m[1, 1] = 1.0f - 2.0f * (xx + zz);
    m[2, 1] = 2.0f * (yz + wx);
    // Column 2
    m[0, 2] = 2.0f * (xz + wy);
    m[1, 2] = 2.0f * (yz - wx);
    m[2, 2] = 1.0f - 2.0f * (xx + yy);
    // Column 3
    m[3, 3] = 1.0f;
    return m;
}

} // namespace fire_engine
