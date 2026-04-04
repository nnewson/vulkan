#pragma once

#include <list>
#include <sstream>
#include <string>

#include <vulkan/vulkan.hpp>

#include <fire_engine/colour.hpp>

namespace fire_engine
{

class Material
{
public:
    static std::list<Material> load_from_file(const std::string& path);

    Material() = default;
    ~Material() = default;

    std::string name;
    Colour3 ambient{0.2f, 0.2f, 0.2f};  // Ka
    Colour3 diffuse{1.0f, 1.0f, 1.0f};  // Kd
    Colour3 specular{0.0f, 0.0f, 0.0f}; // Ks
    Colour3 emissive{0.0f, 0.0f, 0.0f}; // Ke
    float shininess{0.0f};               // Ns
    float ior{1.0f};                     // Ni
    float transparency{0.0f};            // Tr (or d)
    int illum{0};                        // illum
    std::string mapKd;                   // map_Kd
    float roughness{1.0f};               // Pr
    float metallic{0.0f};                // Pm
    float sheen{0.0f};                   // Ps
    float clearcoat{0.0f};               // Pc
    float clearcoatRoughness{0.0f};      // Pcr
    float anisotropy{0.0f};              // aniso
    float anisotropyRotation{0.0f};      // anisor

private:
    static Colour3 parse_colour3(std::istringstream& iss);
    static void trim_comment(std::string& line);
    static void trim(std::string& s);
};

} // namespace fire_engine
