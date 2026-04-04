#pragma once

#include <optional>
#include <sstream>
#include <string>

#include <vulkan/vulkan.hpp>

#include <fire_engine/material.hpp>
#include <fire_engine/math.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

struct Vertex
{
    Vec3 position{};
    Colour3 colour{};
    Vec3 normal{};

    bool operator==(const Vertex& other) const noexcept
    {
        return position == other.position && colour == other.colour && normal == other.normal;
    }

    static vk::VertexInputBindingDescription bindingDesc()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attrDescs()
    {
        return {{
            {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
            {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour)},
            {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
        }};
    }
};

struct FaceVertex
{
    std::size_t position_index{};            // 0-based
    std::optional<std::size_t> normal_index; // 0-based, from vn
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
    static std::vector<FaceVertex> parse_face_vertices(std::istringstream& iss);
    static void trim_comment(std::string& line);
    static void trim(std::string& s);
    void computeNormals();

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Face> triangles;
};

} // namespace fire_engine
