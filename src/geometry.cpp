#include "fire_engine/material.hpp"
#include <fstream>
#include <unordered_map>

#include <fire_engine/geometry.hpp>

namespace fire_engine
{

Geometry Geometry::load_from_file(const std::string& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Failed to open OBJ file: " + path);
    }

    Geometry geometry;
    std::string line;
    std::string current_material;

    while (std::getline(input, line))
    {
        trim_comment(line);
        trim(line);

        if (line.empty())
        {
            continue;
        }

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "v")
        {
            Vec3 v{};
            if (!(iss >> v.x >> v.y >> v.z))
            {
                throw std::runtime_error("Invalid vertex line in OBJ");
            }
            geometry.positions.push_back(v);
        }
        else if (keyword == "usemtl")
        {
            iss >> current_material;
        }
        else if (keyword == "f")
        {
            std::vector<std::size_t> face_indices = parse_face_indices(iss);

            if (face_indices.size() < 3)
            {
                throw std::runtime_error("OBJ face must have at least 3 vertices");
            }

            // Fan triangulation
            for (std::size_t i = 1; i + 1 < face_indices.size(); ++i)
            {
                Face tri;
                tri.material_name = current_material;
                tri.vertices[0] = FaceVertex{face_indices[0]};
                tri.vertices[1] = FaceVertex{face_indices[i]};
                tri.vertices[2] = FaceVertex{face_indices[i + 1]};
                geometry.triangles.push_back(tri);
            }
        }
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
        Colour3 colour{};

        bool operator==(const Key& other) const noexcept
        {
            return position_index == other.position_index && colour == other.colour;
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            auto hash_float = [](float v) -> std::size_t
            { return std::hash<uint32_t>{}(std::bit_cast<uint32_t>(v)); };

            std::size_t h = std::hash<std::size_t>{}(key.position_index);
            h ^= hash_float(key.colour.r) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash_float(key.colour.g) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hash_float(key.colour.b) + 0x9e3779b9 + (h << 6) + (h >> 2);
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
                                   { return m.name == face.material_name; });

            if (it != materials.end())
            {
                face_colour = it->diffuse;
            }
        }

        for (const FaceVertex& fv : face.vertices)
        {
            if (fv.position_index >= positions.size())
            {
                throw std::runtime_error("Face references out-of-range position index");
            }

            Key key{fv.position_index, face_colour};

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

            result.vertices.push_back(
                Vertex{.position = positions[fv.position_index], .colour = face_colour});

            result.indices.push_back(new_index);
        }
    }

    return result;
}

std::vector<std::size_t> Geometry::parse_face_indices(std::istringstream& iss)
{
    std::vector<std::size_t> result;
    std::string token;

    while (iss >> token)
    {
        // Supports:
        // f v1 v2 v3
        // f v1/vt1 v2/vt2 v3/vt3
        // f v1//vn1 v2//vn2 v3//vn3
        // f v1/vt1/vn1 ...
        const auto slash = token.find('/');
        const std::string vertex_part =
            (slash == std::string::npos) ? token : token.substr(0, slash);

        if (vertex_part.empty())
        {
            throw std::runtime_error("Invalid face token in OBJ");
        }

        int obj_index = std::stoi(vertex_part);
        if (obj_index <= 0)
        {
            throw std::runtime_error("Only positive OBJ indices are supported in this loader");
        }

        result.push_back(static_cast<std::size_t>(obj_index - 1)); // OBJ is 1-based
    }

    return result;
}

void Geometry::trim_comment(std::string& line)
{
    const auto pos = line.find('#');
    if (pos != std::string::npos)
    {
        line.erase(pos);
    }
}

void Geometry::trim(std::string& s)
{
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        s.clear();
        return;
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    s = s.substr(first, last - first + 1);
}

} // namespace fire_engine
