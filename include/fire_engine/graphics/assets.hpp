#pragma once

#include <cstddef>
#include <vector>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
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

    void resizeGeometries(std::size_t count)
    {
        geometries_.resize(count);
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

    [[nodiscard]] Geometry& geometry(std::size_t index) noexcept
    {
        return geometries_[index];
    }
    [[nodiscard]] const Geometry& geometry(std::size_t index) const noexcept
    {
        return geometries_[index];
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

private:
    std::vector<Texture> textures_;
    std::vector<Material> materials_;
    std::vector<Geometry> geometries_;
};

} // namespace fire_engine
