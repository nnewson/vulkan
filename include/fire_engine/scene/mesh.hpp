#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

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
    ~Mesh() override;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void load(const std::string& objPath, const std::string& mtlPath,
              const Device& device, const Pipeline& pipeline, Frame& frame);

    void update(const CameraState& input_state, const Transform& transform) override;

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world) override;

private:
    void createUniformBuffers(const Device& device);
    void createDescriptorPool();
    void createDescriptorSets(const Pipeline& pipeline);

    vk::Device vkDevice_;

    Geometry::IndexedRenderData renderData_;
    Material material_;
    Texture texture_;

    vk::Buffer vertexBuf_;
    vk::DeviceMemory vertexMem_;
    vk::Buffer indexBuf_;
    vk::DeviceMemory indexMem_;

    std::vector<vk::Buffer> uniformBufs_;
    std::vector<vk::DeviceMemory> uniformMems_;
    std::vector<void*> uniformMapped_;

    std::vector<vk::Buffer> materialBufs_;
    std::vector<vk::DeviceMemory> materialMems_;
    std::vector<void*> materialMapped_;

    vk::DescriptorPool descPool_;
    std::vector<vk::DescriptorSet> descSets_;
};

} // namespace fire_engine
