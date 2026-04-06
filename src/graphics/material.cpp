#include <fire_engine/core/model_loader.hpp>
#include <fire_engine/graphics/material.hpp>

namespace fire_engine
{

std::list<Material> Material::load_from_file(const std::string& path)
{
    std::list<Material> materials;
    std::optional<Material> current;

    ModelLoader::load_from_file(path,
                                [&](const std::string& keyword, std::istringstream& iss)
                                {
                                    if (keyword == "newmtl")
                                    {
                                        if (current.has_value())
                                        {
                                            materials.push_back(std::move(*current));
                                        }
                                        current = Material{};
                                        std::string n;
                                        iss >> n;
                                        current->name(n);
                                    }
                                    else if (!current.has_value())
                                    {
                                        return;
                                    }
                                    else if (keyword == "Ka")
                                    {
                                        current->ambient(parse_colour3(iss));
                                    }
                                    else if (keyword == "Kd")
                                    {
                                        current->diffuse(parse_colour3(iss));
                                    }
                                    else if (keyword == "Ks")
                                    {
                                        current->specular(parse_colour3(iss));
                                    }
                                    else if (keyword == "Ke")
                                    {
                                        current->emissive(parse_colour3(iss));
                                    }
                                    else if (keyword == "Ns")
                                    {
                                        float v;
                                        iss >> v;
                                        current->shininess(v);
                                    }
                                    else if (keyword == "Ni")
                                    {
                                        float v;
                                        iss >> v;
                                        current->ior(v);
                                    }
                                    else if (keyword == "Tr")
                                    {
                                        float v;
                                        iss >> v;
                                        current->transparency(v);
                                    }
                                    else if (keyword == "d")
                                    {
                                        float d;
                                        iss >> d;
                                        current->transparency(1.0f - d);
                                    }
                                    else if (keyword == "illum")
                                    {
                                        int v;
                                        iss >> v;
                                        current->illum(v);
                                    }
                                    else if (keyword == "map_Kd")
                                    {
                                        std::string v;
                                        iss >> v;
                                        current->mapKd(v);
                                    }
                                    else if (keyword == "Pr")
                                    {
                                        float v;
                                        iss >> v;
                                        current->roughness(v);
                                    }
                                    else if (keyword == "Pm")
                                    {
                                        float v;
                                        iss >> v;
                                        current->metallic(v);
                                    }
                                    else if (keyword == "Ps")
                                    {
                                        float v;
                                        iss >> v;
                                        current->sheen(v);
                                    }
                                    else if (keyword == "Pc")
                                    {
                                        float v;
                                        iss >> v;
                                        current->clearcoat(v);
                                    }
                                    else if (keyword == "Pcr")
                                    {
                                        float v;
                                        iss >> v;
                                        current->clearcoatRoughness(v);
                                    }
                                    else if (keyword == "aniso")
                                    {
                                        float v;
                                        iss >> v;
                                        current->anisotropy(v);
                                    }
                                    else if (keyword == "anisor")
                                    {
                                        float v;
                                        iss >> v;
                                        current->anisotropyRotation(v);
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
