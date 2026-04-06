#pragma once

#include <array>
#include <cstddef>

#include <vulkan/vulkan.hpp>

#include <fire_engine/graphics/colour3.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Vertex
{
public:
    Vertex() = default;
    ~Vertex() = default;

    Vertex(const Vertex&) = default;
    Vertex& operator=(const Vertex&) = default;
    Vertex(Vertex&&) noexcept = default;
    Vertex& operator=(Vertex&&) noexcept = default;

    Vertex(Vec3 position, Colour3 colour, Vec3 normal, float texU, float texV) noexcept
        : position_(position),
          colour_(colour),
          normal_(normal),
          texCoord_{texU, texV}
    {
    }

    [[nodiscard]] Vec3 position() const noexcept
    {
        return position_;
    }
    void position(Vec3 v) noexcept
    {
        position_ = v;
    }

    [[nodiscard]] Colour3 colour() const noexcept
    {
        return colour_;
    }
    void colour(Colour3 c) noexcept
    {
        colour_ = c;
    }

    [[nodiscard]] Vec3 normal() const noexcept
    {
        return normal_;
    }
    void normal(Vec3 v) noexcept
    {
        normal_ = v;
    }

    [[nodiscard]] float texU() const noexcept
    {
        return texCoord_[0];
    }
    void texU(float u) noexcept
    {
        texCoord_[0] = u;
    }

    [[nodiscard]] float texV() const noexcept
    {
        return texCoord_[1];
    }
    void texV(float v) noexcept
    {
        texCoord_[1] = v;
    }

    [[nodiscard]]
    bool operator==(const Vertex& other) const noexcept
    {
        return position_ == other.position_ && colour_ == other.colour_ &&
               normal_ == other.normal_ && texCoord_[0] == other.texCoord_[0] &&
               texCoord_[1] == other.texCoord_[1];
    }

    static vk::VertexInputBindingDescription bindingDesc()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 4> attrDescs()
    {
        return {{
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position_)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour_)},
            {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal_)},
            {3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord_)},
        }};
    }

private:
    Vec3 position_{};
    Colour3 colour_{};
    Vec3 normal_{};
    float texCoord_[2]{0.0f, 0.0f};
};

} // namespace fire_engine
