#pragma once

#include <list>
#include <sstream>
#include <string>

#include <vulkan/vulkan.hpp>

#include <fire_engine/graphics/colour3.hpp>

namespace fire_engine
{

class Material
{
public:
    static std::list<Material> load_from_file(const std::string& path);

    Material() = default;
    ~Material() = default;

    Material(const Material&) = default;
    Material& operator=(const Material&) = default;
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void name(const std::string& name) { name_ = name; }

    [[nodiscard]] Colour3 ambient() const noexcept { return ambient_; }
    void ambient(Colour3 c) noexcept { ambient_ = c; }

    [[nodiscard]] Colour3 diffuse() const noexcept { return diffuse_; }
    void diffuse(Colour3 c) noexcept { diffuse_ = c; }

    [[nodiscard]] Colour3 specular() const noexcept { return specular_; }
    void specular(Colour3 c) noexcept { specular_ = c; }

    [[nodiscard]] Colour3 emissive() const noexcept { return emissive_; }
    void emissive(Colour3 c) noexcept { emissive_ = c; }

    [[nodiscard]] float shininess() const noexcept { return shininess_; }
    void shininess(float v) noexcept { shininess_ = v; }

    [[nodiscard]] float ior() const noexcept { return ior_; }
    void ior(float v) noexcept { ior_ = v; }

    [[nodiscard]] float transparency() const noexcept { return transparency_; }
    void transparency(float v) noexcept { transparency_ = v; }

    [[nodiscard]] int illum() const noexcept { return illum_; }
    void illum(int v) noexcept { illum_ = v; }

    [[nodiscard]] const std::string& mapKd() const noexcept { return mapKd_; }
    void mapKd(const std::string& path) { mapKd_ = path; }

    [[nodiscard]] float roughness() const noexcept { return roughness_; }
    void roughness(float v) noexcept { roughness_ = v; }

    [[nodiscard]] float metallic() const noexcept { return metallic_; }
    void metallic(float v) noexcept { metallic_ = v; }

    [[nodiscard]] float sheen() const noexcept { return sheen_; }
    void sheen(float v) noexcept { sheen_ = v; }

    [[nodiscard]] float clearcoat() const noexcept { return clearcoat_; }
    void clearcoat(float v) noexcept { clearcoat_ = v; }

    [[nodiscard]] float clearcoatRoughness() const noexcept { return clearcoatRoughness_; }
    void clearcoatRoughness(float v) noexcept { clearcoatRoughness_ = v; }

    [[nodiscard]] float anisotropy() const noexcept { return anisotropy_; }
    void anisotropy(float v) noexcept { anisotropy_ = v; }

    [[nodiscard]] float anisotropyRotation() const noexcept { return anisotropyRotation_; }
    void anisotropyRotation(float v) noexcept { anisotropyRotation_ = v; }

private:
    static Colour3 parse_colour3(std::istringstream& iss);

    std::string name_;
    Colour3 ambient_{0.2f, 0.2f, 0.2f};
    Colour3 diffuse_{1.0f, 1.0f, 1.0f};
    Colour3 specular_{0.0f, 0.0f, 0.0f};
    Colour3 emissive_{0.0f, 0.0f, 0.0f};
    float shininess_{0.0f};
    float ior_{1.0f};
    float transparency_{0.0f};
    int illum_{0};
    std::string mapKd_;
    float roughness_{1.0f};
    float metallic_{0.0f};
    float sheen_{0.0f};
    float clearcoat_{0.0f};
    float clearcoatRoughness_{0.0f};
    float anisotropy_{0.0f};
    float anisotropyRotation_{0.0f};
};

} // namespace fire_engine
