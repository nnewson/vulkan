#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/scene/component.hpp>

namespace fire_engine
{

class Device;
class Frame;
class Pipeline;

class Mesh : public Component
{
public:
    Mesh() = default;
    ~Mesh() override = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void load(const Geometry& renderData, const Material& material,
              const std::string& texturePath,
              const Device& device, const Pipeline& pipeline, Frame& frame);

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    void createUniformBuffers(const Device& device);
    void createDescriptorPool();
    void createDescriptorSets(const Pipeline& pipeline);

    const vk::raii::Device* vkDevice_{nullptr};

    Geometry renderData_;
    Material material_;
    Texture texture_;

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
