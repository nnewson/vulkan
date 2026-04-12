#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

class Geometry;
class Material;
class Renderer;
class Device;
class Pipeline;
struct RenderContext;
struct MaterialUBO;

class Object
{
public:
    Object() = default;
    ~Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    Object(Object&&) noexcept = default;
    Object& operator=(Object&&) noexcept = default;

    void addGeometry(const Geometry& geometry);
    void load(const Renderer& renderer);

    [[nodiscard]]
    Mat4 render(const RenderContext& ctx, const Mat4& world);

private:
    struct GeometryBindings
    {
        const Geometry* geometry{nullptr};

        std::vector<vk::raii::Buffer> materialBufs;
        std::vector<vk::raii::DeviceMemory> materialMems;
        std::vector<void*> materialMapped;

        std::vector<vk::raii::DescriptorSet> descSets;
    };

    void createUniformBuffers(const Device& device);
    void createMaterialBuffers(const Device& device, GeometryBindings& bindings);
    void createDescriptorPool(const Device& device);
    void createDescriptorSets(const Device& device, const Pipeline& pipeline);
    static MaterialUBO toMaterialUBO(const Material& mat);

    std::vector<vk::raii::Buffer> uniformBufs_;
    std::vector<vk::raii::DeviceMemory> uniformMems_;
    std::vector<void*> uniformMapped_;

    // Pool declared before bindings so descriptor sets are destroyed first
    vk::raii::DescriptorPool descPool_{nullptr};
    std::vector<GeometryBindings> bindings_;
};

} // namespace fire_engine
