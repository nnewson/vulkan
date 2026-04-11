#include "fire_engine/renderer/renderer.hpp"
#include <fire_engine/graphics/object.hpp>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/frame.hpp>
#include <fire_engine/renderer/pipeline.hpp>
#include <fire_engine/renderer/render_context.hpp>
#include <fire_engine/renderer/swapchain.hpp>
#include <fire_engine/renderer/ubo.hpp>

namespace fire_engine
{

void Object::load(const Geometry& geometry, const Renderer& renderer)
{
    geometry_ = &geometry;

    auto& device = renderer.device();
    createMaterialBuffers(device);
    createUniformBuffers(device);
    createDescriptorPool(device);
    createDescriptorSets(device, renderer.pipeline());
}

void Object::createUniformBuffers(const Device& device)
{
    vk::DeviceSize size = sizeof(UniformBufferObject);
    uniformBufs_.reserve(MAX_FRAMES_IN_FLIGHT);
    uniformMems_.reserve(MAX_FRAMES_IN_FLIGHT);
    uniformMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [uBuf, uMem] = device.createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        uniformMapped_[i] = uMem.mapMemory(0, size);
        uniformBufs_.push_back(std::move(uBuf));
        uniformMems_.push_back(std::move(uMem));
    }
}

void Object::createMaterialBuffers(const Device& device)
{
    const auto& mat = geometry_->material();

    vk::DeviceSize matSize = sizeof(MaterialUBO);
    materialBufs_.reserve(MAX_FRAMES_IN_FLIGHT);
    materialMems_.reserve(MAX_FRAMES_IN_FLIGHT);
    materialMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [mBuf, mMem] = device.createBuffer(matSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        materialMapped_[i] = mMem.mapMemory(0, matSize);
        materialBufs_.push_back(std::move(mBuf));
        materialMems_.push_back(std::move(mMem));

        MaterialUBO matUbo{};
        matUbo.ambient[0] = mat.ambient().r();
        matUbo.ambient[1] = mat.ambient().g();
        matUbo.ambient[2] = mat.ambient().b();
        matUbo.diffuse[0] = mat.diffuse().r();
        matUbo.diffuse[1] = mat.diffuse().g();
        matUbo.diffuse[2] = mat.diffuse().b();
        matUbo.specular[0] = mat.specular().r();
        matUbo.specular[1] = mat.specular().g();
        matUbo.specular[2] = mat.specular().b();
        matUbo.emissive[0] = mat.emissive().r();
        matUbo.emissive[1] = mat.emissive().g();
        matUbo.emissive[2] = mat.emissive().b();
        matUbo.shininess = mat.shininess();
        matUbo.ior = mat.ior();
        matUbo.transparency = mat.transparency();
        matUbo.illum = mat.illum();
        matUbo.roughness = mat.roughness();
        matUbo.metallic = mat.metallic();
        matUbo.sheen = mat.sheen();
        matUbo.clearcoat = mat.clearcoat();
        matUbo.clearcoatRoughness = mat.clearcoatRoughness();
        matUbo.anisotropy = mat.anisotropy();
        matUbo.anisotropyRotation = mat.anisotropyRotation();
        memcpy(materialMapped_[i], &matUbo, sizeof(matUbo));
    }
}

void Object::createDescriptorPool(const Device& device)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2)},
        {vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
    }};
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                    static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), poolSizes);
    descPool_ = vk::raii::DescriptorPool(device.device(), ci);
}

void Object::createDescriptorSets(const Device& device, const Pipeline& pipeline)
{
    const auto& mat = geometry_->material();

    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 pipeline.descriptorSetLayout());
    vk::DescriptorSetAllocateInfo ai(*descPool_, layouts);
    auto sets = device.device().allocateDescriptorSets(ai);
    descSets_.clear();
    descSets_.reserve(sets.size());
    for (auto& s : sets)
        descSets_.push_back(std::move(s));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo uboBufInfo(*uniformBufs_[i], 0, sizeof(UniformBufferObject));
        vk::DescriptorBufferInfo matBufInfo(*materialBufs_[i], 0, sizeof(MaterialUBO));
        vk::DescriptorImageInfo texInfo(mat.texture().sampler(), mat.texture().view(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);

        std::array<vk::WriteDescriptorSet, 3> writes = {{
            {*descSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboBufInfo},
            {*descSets_[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &matBufInfo},
            {*descSets_[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
        }};
        device.device().updateDescriptorSets(writes, {});
    }
}

Mat4 Object::render(const RenderContext& ctx, const Mat4& world)
{
    auto extent = ctx.swapchain.extent();

    // Write UBO
    UniformBufferObject ubo{};
    ubo.model = world;
    ubo.view = Mat4::lookAt(ctx.cameraPosition, ctx.cameraTarget, {0, 1, 0});
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = ctx.cameraPosition.x();
    ubo.cameraPos[1] = ctx.cameraPosition.y();
    ubo.cameraPos[2] = ctx.cameraPosition.z();
    ubo.cameraPos[3] = 0.0f;
    memcpy(uniformMapped_[ctx.currentFrame], &ubo, sizeof(ubo));

    // Record draw commands using shared Geometry buffers
    auto cmd = ctx.commandBuffer;
    cmd.bindVertexBuffers(0, geometry_->vertexBuffer(), {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(geometry_->indexBuffer(), 0, vk::IndexType::eUint16);
    vk::DescriptorSet ds = *descSets_[ctx.currentFrame];
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ctx.pipeline.pipelineLayout(), 0, ds,
                           {});
    cmd.drawIndexed(geometry_->indexCount(), 1, 0, 0, 0);

    return world;
}

} // namespace fire_engine
