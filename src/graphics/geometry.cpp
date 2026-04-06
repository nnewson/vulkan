#include <stdexcept>
#include <unordered_map>

#include <tiny_obj_loader.h>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

Geometry Geometry::load_from_file(const std::string& path)
{
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config))
    {
        throw std::runtime_error("Failed to load OBJ: " + reader.Error());
    }

    Geometry geometry;

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    // Copy positions (3 floats per vertex)
    for (std::size_t i = 0; i < attrib.vertices.size(); i += 3)
    {
        geometry.positions.push_back(
            {attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]});
    }

    // Copy normals (3 floats per normal)
    for (std::size_t i = 0; i < attrib.normals.size(); i += 3)
    {
        geometry.normals.push_back(
            {attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]});
    }

    // Copy texcoords (2 floats per texcoord)
    for (std::size_t i = 0; i < attrib.texcoords.size(); i += 2)
    {
        geometry.texcoords.push_back({attrib.texcoords[i], attrib.texcoords[i + 1]});
    }

    // Build triangles from shapes
    for (const auto& shape : shapes)
    {
        std::size_t index_offset = 0;

        for (std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f)
        {
            Face tri;

            int mat_id = shape.mesh.material_ids[f];
            if (mat_id >= 0 && static_cast<std::size_t>(mat_id) < materials.size())
            {
                tri.material_name = materials[mat_id].name;
            }

            for (std::size_t v = 0; v < 3; ++v)
            {
                const auto& idx = shape.mesh.indices[index_offset + v];

                FaceVertex fv;
                fv.position_index = static_cast<std::size_t>(idx.vertex_index);

                if (idx.texcoord_index >= 0)
                {
                    fv.texcoord_index = static_cast<std::size_t>(idx.texcoord_index);
                }

                if (idx.normal_index >= 0)
                {
                    fv.normal_index = static_cast<std::size_t>(idx.normal_index);
                }

                tri.vertices[v] = fv;
            }

            geometry.triangles.push_back(tri);
            index_offset += 3;
        }
    }

    if (geometry.normals.empty())
    {
        geometry.computeNormals();
    }

    return geometry;
}

Geometry::IndexedRenderData
Geometry::to_coloured_indexed_geometry(const std::list<Material> materials) const
{
    IndexedRenderData result;

    struct Key
    {
        std::size_t position_index{};
        std::size_t normal_index{};
        std::size_t texcoord_index{};
        Colour3 colour{};

        bool operator==(const Key& other) const noexcept
        {
            return position_index == other.position_index && normal_index == other.normal_index &&
                   texcoord_index == other.texcoord_index && colour == other.colour;
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            auto hash_float = [](float v) -> std::size_t
            { return std::hash<uint32_t>{}(std::bit_cast<uint32_t>(v)); };

            std::size_t h = std::hash<std::size_t>{}(key.position_index);
            h ^= std::hash<std::size_t>{}(key.normal_index) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<std::size_t>{}(key.texcoord_index) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash_float(key.colour.r()) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash_float(key.colour.g()) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash_float(key.colour.b()) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<Key, uint16_t, KeyHash> vertex_map;

    for (const Face& face : triangles)
    {
        Colour3 face_colour{1.0f, 1.0f, 1.0f};

        if (!face.material_name.empty())
        {
            auto it = std::find_if(materials.begin(), materials.end(), [&face](const Material& m)
                                   { return m.name() == face.material_name; });

            if (it != materials.end())
            {
                face_colour = it->diffuse();
            }
        }

        for (const FaceVertex& fv : face.vertices)
        {
            if (fv.position_index >= positions.size())
            {
                throw std::runtime_error("Face references out-of-range position index");
            }

            std::size_t ni = fv.normal_index.value_or(fv.position_index);
            if (ni >= normals.size())
            {
                throw std::runtime_error("Face references out-of-range normal index");
            }

            // Use texcoord index if available, otherwise 0 (sentinel for no UVs)
            std::size_t ti = fv.texcoord_index.value_or(0);

            Key key{fv.position_index, ni, ti, face_colour};

            auto it = vertex_map.find(key);
            if (it != vertex_map.end())
            {
                result.indices.push_back(it->second);
                continue;
            }

            if (result.vertices.size() >=
                static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()) + 1ULL)
            {
                throw std::runtime_error("Too many vertices for uint16_t indices");
            }

            const uint16_t new_index = static_cast<uint16_t>(result.vertices.size());
            vertex_map.emplace(key, new_index);

            float u = 0.0f, v = 0.0f;
            if (fv.texcoord_index.has_value() && ti < texcoords.size())
            {
                u = texcoords[ti][0];
                v = texcoords[ti][1];
            }

            result.vertices.push_back(Vertex{
                positions[fv.position_index],
                face_colour,
                normals[ni],
                u,
                v,
            });

            result.indices.push_back(new_index);
        }
    }

    return result;
}

void Geometry::computeNormals()
{
    normals.resize(positions.size(), {0.0f, 0.0f, 0.0f});

    for (const Face& tri : triangles)
    {
        const Vec3& p0 = positions[tri.vertices[0].position_index];
        const Vec3& p1 = positions[tri.vertices[1].position_index];
        const Vec3& p2 = positions[tri.vertices[2].position_index];

        Vec3 faceNormal = Vec3::crossProduct(p1 - p0, p2 - p0);

        for (const FaceVertex& fv : tri.vertices)
        {
            Vec3& n = normals[fv.position_index];
            n += faceNormal;
        }
    }

    for (Vec3& n : normals)
    {
        n = n.normalise();
    }
}

} // namespace fire_engine
