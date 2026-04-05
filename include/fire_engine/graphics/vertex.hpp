#pragma once

#include <array>

#include <vulkan/vulkan.hpp>

#include <fire_engine/graphics/colour3.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

struct Vertex
{
    Vec3 position{};
    Colour3 colour{};
    Vec3 normal{};
    float texCoord[2]{0.0f, 0.0f};

    bool operator==(const Vertex& other) const noexcept
    {
        return position == other.position && colour == other.colour && normal == other.normal &&
               texCoord[0] == other.texCoord[0] && texCoord[1] == other.texCoord[1];
    }

    static vk::VertexInputBindingDescription bindingDesc()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 4> attrDescs()
    {
        return {{
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour)},
            {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
            {3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)},
        }};
    }
};

} // namespace fire_engine
