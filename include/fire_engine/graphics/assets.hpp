#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <fire_engine/animation/animation.hpp>
#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/skin.hpp>
#include <fire_engine/graphics/texture.hpp>

namespace fire_engine
{

class Assets
{
public:
    Assets() = default;
    ~Assets() = default;

    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;
    Assets(Assets&&) noexcept = default;
    Assets& operator=(Assets&&) noexcept = default;

    void resizeTextures(std::size_t count)
    {
        textures_.resize(count);
    }

    void resizeMaterials(std::size_t count)
    {
        materials_.resize(count);
    }

    void reserveMaterials(std::size_t count)
    {
        materials_.reserve(count);
    }

    void resizeGeometries(std::size_t count)
    {
        geometries_.resize(count);
    }

    void resizeSkins(std::size_t count)
    {
        skins_.resize(count);
    }

    void resizeAnimations(std::size_t count)
    {
        animations_.resize(count);
    }

    [[nodiscard]] Texture& texture(std::size_t index) noexcept
    {
        return textures_[index];
    }
    [[nodiscard]] const Texture& texture(std::size_t index) const noexcept
    {
        return textures_[index];
    }

    [[nodiscard]] Material& material(std::size_t index) noexcept
    {
        return materials_[index];
    }
    [[nodiscard]] const Material& material(std::size_t index) const noexcept
    {
        return materials_[index];
    }

    [[nodiscard]] Material& addMaterial(Material material)
    {
        materials_.push_back(std::move(material));
        return materials_.back();
    }

    [[nodiscard]] Geometry& geometry(std::size_t index) noexcept
    {
        return geometries_[index];
    }
    [[nodiscard]] const Geometry& geometry(std::size_t index) const noexcept
    {
        return geometries_[index];
    }

    [[nodiscard]] Skin& skin(std::size_t index) noexcept
    {
        return skins_[index];
    }
    [[nodiscard]] const Skin& skin(std::size_t index) const noexcept
    {
        return skins_[index];
    }

    [[nodiscard]] std::size_t textureCount() const noexcept
    {
        return textures_.size();
    }
    [[nodiscard]] std::size_t materialCount() const noexcept
    {
        return materials_.size();
    }
    [[nodiscard]] std::size_t geometryCount() const noexcept
    {
        return geometries_.size();
    }
    [[nodiscard]] std::size_t skinCount() const noexcept
    {
        return skins_.size();
    }

    [[nodiscard]] Animation& animation(std::size_t index) noexcept
    {
        return animations_[index];
    }
    [[nodiscard]] const Animation& animation(std::size_t index) const noexcept
    {
        return animations_[index];
    }
    [[nodiscard]] std::size_t animationCount() const noexcept
    {
        return animations_.size();
    }

private:
    std::vector<Texture> textures_;
    std::vector<Material> materials_;
    std::vector<Geometry> geometries_;
    std::vector<Skin> skins_;
    std::vector<Animation> animations_;
};

} // namespace fire_engine
