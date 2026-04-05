#pragma once

#include <optional>
#include <sstream>
#include <string>

#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/vertex.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Geometry
{
public:
    static Geometry load_from_file(const std::string& path);

    Geometry() = default;
    ~Geometry() = default;

    Geometry(const Geometry&) = default;
    Geometry& operator=(const Geometry&) = default;
    Geometry(Geometry&&) noexcept = default;
    Geometry& operator=(Geometry&&) noexcept = default;

    struct IndexedRenderData
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };

    Geometry::IndexedRenderData
    to_coloured_indexed_geometry(const std::list<Material> materials) const;

private:
    struct FaceVertex
    {
        std::size_t position_index{};
        std::optional<std::size_t> texcoord_index;
        std::optional<std::size_t> normal_index;
    };

    struct Face
    {
        std::array<FaceVertex, 3> vertices{};
        std::string material_name;
    };

    static std::vector<FaceVertex> parse_face_vertices(std::istringstream& iss);
    void computeNormals();

    std::vector<Vec3> positions;
    std::vector<std::array<float, 2>> texcoords;
    std::vector<Vec3> normals;
    std::vector<Face> triangles;
};

} // namespace fire_engine
