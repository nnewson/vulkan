#pragma once

#include <cstdint>
#include <vector>

#include <fire_engine/graphics/vertex.hpp>

namespace fire_engine
{

class Geometry
{
public:
    Geometry() = default;
    ~Geometry() = default;

    Geometry(const Geometry&) = default;
    Geometry& operator=(const Geometry&) = default;
    Geometry(Geometry&&) noexcept = default;
    Geometry& operator=(Geometry&&) noexcept = default;

    [[nodiscard]] const std::vector<Vertex>& vertices() const noexcept
    {
        return vertices_;
    }
    void vertices(std::vector<Vertex> v) noexcept
    {
        vertices_ = std::move(v);
    }

    [[nodiscard]] const std::vector<uint16_t>& indices() const noexcept
    {
        return indices_;
    }
    void indices(std::vector<uint16_t> v) noexcept
    {
        indices_ = std::move(v);
    }

private:
    std::vector<Vertex> vertices_;
    std::vector<uint16_t> indices_;
};

} // namespace fire_engine
