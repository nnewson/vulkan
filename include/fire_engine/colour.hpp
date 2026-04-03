#pragma once

namespace fire_engine
{

struct Colour3
{
    float r{};
    float g{};
    float b{};

    bool operator==(const Colour3& other) const noexcept
    {
        return r == other.r && g == other.g && b == other.b;
    }
};

} // namespace fire_engine
