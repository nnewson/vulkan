#include <fstream>
#include <map>
#include <stdexcept>

#include <tiny_obj_loader.h>

#include <fire_engine/graphics/material.hpp>

namespace fire_engine
{

std::list<Material> Material::load_from_file(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        throw std::runtime_error("Failed to open MTL file: " + path);
    }

    std::map<std::string, int> material_map;
    std::vector<tinyobj::material_t> tinyobj_materials;
    std::string warning;
    std::string err;

    tinyobj::LoadMtl(&material_map, &tinyobj_materials, &ifs, &warning, &err);

    std::list<Material> materials;

    for (const auto& src : tinyobj_materials)
    {
        if (src.name.empty())
        {
            continue;
        }
        Material m;
        m.name(src.name);
        m.ambient({src.ambient[0], src.ambient[1], src.ambient[2]});
        m.diffuse({src.diffuse[0], src.diffuse[1], src.diffuse[2]});
        m.specular({src.specular[0], src.specular[1], src.specular[2]});
        m.emissive({src.emission[0], src.emission[1], src.emission[2]});
        m.shininess(src.shininess);
        m.ior(src.ior);
        m.transparency(1.0f - src.dissolve);
        m.illum(src.illum);
        m.mapKd(src.diffuse_texname);
        m.roughness(src.roughness);
        m.metallic(src.metallic);
        m.sheen(src.sheen);
        m.clearcoat(src.clearcoat_thickness);
        m.clearcoatRoughness(src.clearcoat_roughness);
        m.anisotropy(src.anisotropy);
        m.anisotropyRotation(src.anisotropy_rotation);
        materials.push_back(std::move(m));
    }

    return materials;
}

} // namespace fire_engine
