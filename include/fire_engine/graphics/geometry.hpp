#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/vertex.hpp>
#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

class Renderer;
class Device;
class Frame;
class Pipeline;
struct RenderContext;

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

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world);

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

    [[nodiscard]] Material& material() noexcept
    {
        return material_;
    }
    [[nodiscard]] const Material& material() const noexcept
    {
        return material_;
    }
    void material(Material m) noexcept
    {
        material_ = std::move(m);
    }

private:
    void createUniformBuffers(const Device& device);
    void createDescriptorPool();
    void createDescriptorSets(const Pipeline& pipeline);

    const vk::raii::Device* vkDevice_{nullptr};

    std::vector<Vertex> vertices_;
    std::vector<uint16_t> indices_;
    Material material_;

    vk::raii::Buffer vertexBuf_{nullptr};
    vk::raii::DeviceMemory vertexMem_{nullptr};
    vk::raii::Buffer indexBuf_{nullptr};
    vk::raii::DeviceMemory indexMem_{nullptr};

    std::vector<vk::raii::Buffer> uniformBufs_;
    std::vector<vk::raii::DeviceMemory> uniformMems_;
    std::vector<void*> uniformMapped_;

    std::vector<vk::raii::Buffer> materialBufs_;
    std::vector<vk::raii::DeviceMemory> materialMems_;
    std::vector<void*> materialMapped_;

    // Descriptor sets declared after pool so they're destroyed first
    vk::raii::DescriptorPool descPool_{nullptr};
    std::vector<vk::raii::DescriptorSet> descSets_;
};

} // namespace fire_engine
