#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/graphics/vertex.hpp>

namespace fire_engine
{

class Material;
class Renderer;
class Device;

class Geometry
{
public:
    Geometry() = default;
    ~Geometry() = default;

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;
    Geometry(Geometry&&) noexcept = default;
    Geometry& operator=(Geometry&&) noexcept = default;

    void load(const Renderer& renderer);

    [[nodiscard]] bool loaded() const noexcept
    {
        return vkDevice_ != nullptr;
    }

    [[nodiscard]] const std::vector<Vertex>& vertices() const noexcept
    {
        return vertices_;
    }
    void vertices(std::vector<Vertex> v) noexcept
    {
        vertices_ = std::move(v);
    }

    [[nodiscard]] const std::vector<uint16_t>& indices() const noexcept
    {
        return indices_;
    }
    void indices(std::vector<uint16_t> v) noexcept
    {
        indices_ = std::move(v);
    }

    [[nodiscard]] const Material& material() const noexcept
    {
        return *material_;
    }
    void material(const Material* m) noexcept
    {
        material_ = m;
    }

    [[nodiscard]] vk::Buffer vertexBuffer() const noexcept
    {
        return *vertexBuf_;
    }

    [[nodiscard]] vk::Buffer indexBuffer() const noexcept
    {
        return *indexBuf_;
    }

    [[nodiscard]] uint32_t indexCount() const noexcept
    {
        return static_cast<uint32_t>(indices_.size());
    }

    [[nodiscard]] const std::vector<std::vector<Vec3>>& morphPositions() const noexcept
    {
        return morphPositions_;
    }
    void morphPositions(std::vector<std::vector<Vec3>> v) noexcept
    {
        morphPositions_ = std::move(v);
    }

    [[nodiscard]] const std::vector<std::vector<Vec3>>& morphNormals() const noexcept
    {
        return morphNormals_;
    }
    void morphNormals(std::vector<std::vector<Vec3>> v) noexcept
    {
        morphNormals_ = std::move(v);
    }

    [[nodiscard]] std::size_t morphTargetCount() const noexcept
    {
        return morphPositions_.size();
    }

private:
    const vk::raii::Device* vkDevice_{nullptr};

    std::vector<Vertex> vertices_;
    std::vector<uint16_t> indices_;
    const Material* material_{nullptr};

    std::vector<std::vector<Vec3>> morphPositions_;
    std::vector<std::vector<Vec3>> morphNormals_;

    vk::raii::Buffer vertexBuf_{nullptr};
    vk::raii::DeviceMemory vertexMem_{nullptr};
    vk::raii::Buffer indexBuf_{nullptr};
    vk::raii::DeviceMemory indexMem_{nullptr};
};

} // namespace fire_engine
