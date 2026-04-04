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
            if (!(iss >> v.x_ >> v.y_ >> v.z_))
            {
                throw std::runtime_error("Invalid vertex line in OBJ");
            }
            geometry.positions.push_back(v);
        }
        else if (keyword == "vn")
        {
            Vec3 n{};
            if (!(iss >> n.x_ >> n.y_ >> n.z_))
            {
                throw std::runtime_error("Invalid vertex normal line in OBJ");
            }
            geometry.normals.push_back(n);
        }
        else if (keyword == "usemtl")
        {
            iss >> current_material;
        }
        else if (keyword == "f")
        {
            std::vector<FaceVertex> face_verts = parse_face_vertices(iss);

            if (face_verts.size() < 3)
            {
                throw std::runtime_error("OBJ face must have at least 3 vertices");
            }

            // Fan triangulation
            for (std::size_t i = 1; i + 1 < face_verts.size(); ++i)
            {
                Face tri;
                tri.material_name = current_material;
                tri.vertices[0] = face_verts[0];
                tri.vertices[1] = face_verts[i];
                tri.vertices[2] = face_verts[i + 1];
                geometry.triangles.push_back(tri);
            }
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
        Colour3 colour{};

        bool operator==(const Key& other) const noexcept
        {
            return position_index == other.position_index && normal_index == other.normal_index &&
                   colour == other.colour;
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

            std::size_t ni = fv.normal_index.value_or(fv.position_index);
            if (ni >= normals.size())
            {
                throw std::runtime_error("Face references out-of-range normal index");
            }

            Key key{fv.position_index, ni, face_colour};

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

            result.vertices.push_back(Vertex{
                .position = positions[fv.position_index],
                .colour = face_colour,
                .normal = normals[ni],
            });

            result.indices.push_back(new_index);
        }
    }

    return result;
}

std::vector<FaceVertex> Geometry::parse_face_vertices(std::istringstream& iss)
{
    std::vector<FaceVertex> result;
    std::string token;

    while (iss >> token)
    {
        FaceVertex fv{};

        // Supports: f v, f v/vt, f v//vn, f v/vt/vn
        const auto slash1 = token.find('/');
        const std::string pos_part =
            (slash1 == std::string::npos) ? token : token.substr(0, slash1);

        if (pos_part.empty())
        {
            throw std::runtime_error("Invalid face token in OBJ");
        }

        int obj_index = std::stoi(pos_part);
        if (obj_index <= 0)
        {
            throw std::runtime_error("Only positive OBJ indices are supported in this loader");
        }
        fv.position_index = static_cast<std::size_t>(obj_index - 1);

        // Parse normal index if present (third component: v/vt/vn or v//vn)
        if (slash1 != std::string::npos)
        {
            const auto slash2 = token.find('/', slash1 + 1);
            if (slash2 != std::string::npos)
            {
                const std::string normal_part = token.substr(slash2 + 1);
                if (!normal_part.empty())
                {
                    int ni = std::stoi(normal_part);
                    if (ni > 0)
                        fv.normal_index = static_cast<std::size_t>(ni - 1);
                }
            }
        }

        result.push_back(fv);
    }

    return result;
}

void Geometry::computeNormals()
{
    normals.resize(positions.size(), Vec3{0.0f, 0.0f, 0.0f});

    for (const Face& tri : triangles)
    {
        const Vec3& p0 = positions[tri.vertices[0].position_index];
        const Vec3& p1 = positions[tri.vertices[1].position_index];
        const Vec3& p2 = positions[tri.vertices[2].position_index];

        Vec3 faceNormal = Vec3::crossProduct(p1 - p0, p2 - p0);

        for (const FaceVertex& fv : tri.vertices)
        {
            Vec3& n = normals[fv.position_index];
            n.x_ += faceNormal.x_;
            n.y_ += faceNormal.y_;
            n.z_ += faceNormal.z_;
        }
    }

    for (Vec3& n : normals)
    {
        n = n.normalise();
    }
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
