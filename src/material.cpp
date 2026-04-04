#include <fstream>

#include <fire_engine/material.hpp>

namespace fire_engine
{

std::list<Material> Material::load_from_file(const std::string& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Failed to open MTL file: " + path);
    }

    std::list<Material> materials;
    std::optional<Material> current;
    std::string line;

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
            continue;
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
    }

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

void Material::trim_comment(std::string& line)
{
    const auto pos = line.find('#');
    if (pos != std::string::npos)
    {
        line.erase(pos);
    }
}

void Material::trim(std::string& s)
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
