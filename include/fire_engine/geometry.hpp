#pragma once

#include <sstream>
#include <string>

#include <vulkan/vulkan.hpp>

#include <fire_engine/material.hpp>
#include <fire_engine/math.hpp>

namespace fire_engine
{

struct Vertex
{
    Vec3 position{};
    Colour3 colour{};

    bool operator==(const Vertex& other) const noexcept
    {
        return position == other.position && colour == other.colour;
    }

    static vk::VertexInputBindingDescription bindingDesc()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2> attrDescs()
    {
        return {{
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour)},
        }};
    }
};

struct FaceVertex
{
    std::size_t position_index{}; // 0-based
};

struct Face
{
    std::array<FaceVertex, 3> vertices{};
    std::string material_name;
};

class Geometry
{
public:
    static Geometry load_from_file(const std::string& path);

    Geometry() = default;
    ~Geometry() = default;

    struct IndexedRenderData
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };

    Geometry::IndexedRenderData
    to_coloured_indexed_geometry(const std::list<Material> materials) const;

private:
    static std::vector<std::size_t> parse_face_indices(std::istringstream& iss);
    static void trim_comment(std::string& line);
    static void trim(std::string& s);

    std::vector<Vec3> positions;
    std::vector<Face> triangles;
};

} // namespace fire_engine
