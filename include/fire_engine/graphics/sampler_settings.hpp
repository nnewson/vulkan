#pragma once

namespace fire_engine
{

enum class WrapMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge
};

enum class FilterMode
{
    Nearest,
    Linear
};

struct SamplerSettings
{
    WrapMode wrapS{WrapMode::Repeat};
    WrapMode wrapT{WrapMode::Repeat};
    FilterMode magFilter{FilterMode::Linear};
    FilterMode minFilter{FilterMode::Linear};

    [[nodiscard]] constexpr bool operator==(const SamplerSettings&) const noexcept = default;
};

} // namespace fire_engine
