#pragma once

#include <vulkan/vulkan.hpp>

#include <fire_engine/math.hpp>

namespace fire_engine
{

struct Vertex
{
    Vec3 pos;
    Vec3 color;

    static vk::VertexInputBindingDescription bindingDesc()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2> attrDescs()
    {
        return {{
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)},
        }};
    }
};

} // namespace fire_engine
