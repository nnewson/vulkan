#pragma once

#include <string>

#include <fire_engine/graphics/colour3.hpp>
#include <fire_engine/graphics/texture.hpp>

namespace fire_engine
{

class Material
{
public:
    Material() = default;
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    void loadTexture(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physDevice,
                     vk::CommandPool cmdPool, const vk::raii::Queue& queue);

    [[nodiscard]] const Texture& texture() const noexcept
    {
        return texture_;
    }

    [[nodiscard]] const std::string& name() const noexcept
    {
        return name_;
    }
    void name(const std::string& name)
    {
        name_ = name;
    }

    [[nodiscard]] Colour3 ambient() const noexcept
    {
        return ambient_;
    }
    void ambient(Colour3 c) noexcept
    {
        ambient_ = c;
    }

    [[nodiscard]] Colour3 diffuse() const noexcept
    {
        return diffuse_;
    }
    void diffuse(Colour3 c) noexcept
    {
        diffuse_ = c;
    }

    [[nodiscard]] Colour3 specular() const noexcept
    {
        return specular_;
    }
    void specular(Colour3 c) noexcept
    {
        specular_ = c;
    }

    [[nodiscard]] Colour3 emissive() const noexcept
    {
        return emissive_;
    }
    void emissive(Colour3 c) noexcept
    {
        emissive_ = c;
    }

    [[nodiscard]] float shininess() const noexcept
    {
        return shininess_;
    }
    void shininess(float v) noexcept
    {
        shininess_ = v;
    }

    [[nodiscard]] float ior() const noexcept
    {
        return ior_;
    }
    void ior(float v) noexcept
    {
        ior_ = v;
    }

    [[nodiscard]] float transparency() const noexcept
    {
        return transparency_;
    }
    void transparency(float v) noexcept
    {
        transparency_ = v;
    }

    [[nodiscard]] int illum() const noexcept
    {
        return illum_;
    }
    void illum(int v) noexcept
    {
        illum_ = v;
    }

    [[nodiscard]] const std::string& mapKd() const noexcept
    {
        return mapKd_;
    }
    void mapKd(const std::string& path)
    {
        mapKd_ = path;
    }

    [[nodiscard]] float roughness() const noexcept
    {
        return roughness_;
    }
    void roughness(float v) noexcept
    {
        roughness_ = v;
    }

    [[nodiscard]] float metallic() const noexcept
    {
        return metallic_;
    }
    void metallic(float v) noexcept
    {
        metallic_ = v;
    }

    [[nodiscard]] float sheen() const noexcept
    {
        return sheen_;
    }
    void sheen(float v) noexcept
    {
        sheen_ = v;
    }

    [[nodiscard]] float clearcoat() const noexcept
    {
        return clearcoat_;
    }
    void clearcoat(float v) noexcept
    {
        clearcoat_ = v;
    }

    [[nodiscard]] float clearcoatRoughness() const noexcept
    {
        return clearcoatRoughness_;
    }
    void clearcoatRoughness(float v) noexcept
    {
        clearcoatRoughness_ = v;
    }

    [[nodiscard]] float anisotropy() const noexcept
    {
        return anisotropy_;
    }
    void anisotropy(float v) noexcept
    {
        anisotropy_ = v;
    }

    [[nodiscard]] float anisotropyRotation() const noexcept
    {
        return anisotropyRotation_;
    }
    void anisotropyRotation(float v) noexcept
    {
        anisotropyRotation_ = v;
    }

private:
    std::string name_;
    Colour3 ambient_{};
    Colour3 diffuse_{};
    Colour3 specular_{};
    Colour3 emissive_{};
    float shininess_{0.0f};
    float ior_{0.0f};
    float transparency_{0.0f};
    int illum_{0};
    std::string mapKd_;
    float roughness_{0.0f};
    float metallic_{0.0f};
    float sheen_{0.0f};
    float clearcoat_{0.0f};
    float clearcoatRoughness_{0.0f};
    float anisotropy_{0.0f};
    float anisotropyRotation_{0.0f};

    Texture texture_;
};

} // namespace fire_engine
