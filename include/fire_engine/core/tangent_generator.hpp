#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <fire_engine/math/vec2.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/math/vec4.hpp>

namespace fire_engine
{

struct TangentGenerationResult
{
    bool succeeded{false};
    std::string reason{};
    std::vector<Vec4> tangents{};
};

class TangentGenerator
{
public:
    TangentGenerator() = delete;

    [[nodiscard]]
    static TangentGenerationResult generate(const std::vector<Vec3>& positions,
                                            const std::vector<Vec3>& normals,
                                            const std::vector<Vec2>& texcoords,
                                            const std::vector<uint32_t>& indices);
};

} // namespace fire_engine
