#include <fire_engine/graphics/model_loader.hpp>
#include <fire_engine/graphics/material.hpp>

namespace fire_engine
{

std::list<Material> Material::load_from_file(const std::string& path)
{
    std::list<Material> materials;
    std::optional<Material> current;

    ModelLoader::parse_file(path, [&](const std::string& keyword, std::istringstream& iss)
    {
        if (keyword == "newmtl")
        {
            if (current.has_value())
            {
                materials.push_back(std::move(*current));
            }
            current = Material{};
            iss >> current->name;
        }
        else if (!current.has_value())
        {
            return;
        }
        else if (keyword == "Ka")
        {
            current->ambient = parse_colour3(iss);
        }
        else if (keyword == "Kd")
        {
            current->diffuse = parse_colour3(iss);
        }
        else if (keyword == "Ks")
        {
            current->specular = parse_colour3(iss);
        }
        else if (keyword == "Ke")
        {
            current->emissive = parse_colour3(iss);
        }
        else if (keyword == "Ns")
        {
            iss >> current->shininess;
        }
        else if (keyword == "Ni")
        {
            iss >> current->ior;
        }
        else if (keyword == "Tr")
        {
            iss >> current->transparency;
        }
        else if (keyword == "d")
        {
            float d;
            iss >> d;
            current->transparency = 1.0f - d;
        }
        else if (keyword == "illum")
        {
            iss >> current->illum;
        }
        else if (keyword == "map_Kd")
        {
            iss >> current->mapKd;
        }
        else if (keyword == "Pr")
        {
            iss >> current->roughness;
        }
        else if (keyword == "Pm")
        {
            iss >> current->metallic;
        }
        else if (keyword == "Ps")
        {
            iss >> current->sheen;
        }
        else if (keyword == "Pc")
        {
            iss >> current->clearcoat;
        }
        else if (keyword == "Pcr")
        {
            iss >> current->clearcoatRoughness;
        }
        else if (keyword == "aniso")
        {
            iss >> current->anisotropy;
        }
        else if (keyword == "anisor")
        {
            iss >> current->anisotropyRotation;
        }
    });

    if (current.has_value())
    {
        materials.push_back(std::move(*current));
    }

    return materials;
}

Colour3 Material::parse_colour3(std::istringstream& iss)
{
    Colour3 c{};
    if (!(iss >> c))
    {
        throw std::runtime_error("Invalid color in MTL");
    }
    return c;
}

} // namespace fire_engine
