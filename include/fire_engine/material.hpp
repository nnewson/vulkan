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
    float shininess{0.0f};              // Ns

private:
    static Colour3 parse_colour3(std::istringstream& iss);
    static void trim_comment(std::string& line);
    static void trim(std::string& s);
};

} // namespace fire_engine
