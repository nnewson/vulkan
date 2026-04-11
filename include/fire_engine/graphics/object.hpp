#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

class Geometry;
class Renderer;
class Device;
class Pipeline;
struct RenderContext;

class Object
{
public:
    Object() = default;
    ~Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    Object(Object&&) noexcept = default;
    Object& operator=(Object&&) noexcept = default;

    void load(const Geometry& geometry, const Renderer& renderer);

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world);

private:
    void createUniformBuffers(const Device& device);
    void createMaterialBuffers(const Device& device);
    void createDescriptorPool(const Device& device);
    void createDescriptorSets(const Device& device, const Pipeline& pipeline);

    const Geometry* geometry_{nullptr};

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
