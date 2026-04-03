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
        else if (keyword == "Ns")
        {
            iss >> current->shininess;
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
    if (!(iss >> c.r >> c.g >> c.b))
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
