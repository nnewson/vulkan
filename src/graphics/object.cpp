#include "fire_engine/render/renderer.hpp"
#include <fire_engine/graphics/object.hpp>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/skin.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/render/device.hpp>
#include <fire_engine/render/frame.hpp>
#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/render_context.hpp>
#include <fire_engine/render/swapchain.hpp>
#include <fire_engine/render/ubo.hpp>

namespace fire_engine
{

void Object::addGeometry(const Geometry& geometry)
{
    auto& binding = bindings_.emplace_back();
    binding.geometry = &geometry;
}

void Object::load(const Renderer& renderer)
{
    auto& device = renderer.device();

    createUniformBuffers(device);

    for (auto& binding : bindings_)
    {
        createMaterialBuffers(device, binding);
        createSkinBuffers(device, binding);
    }

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

void Object::createMaterialBuffers(const Device& device, GeometryBindings& binding)
{
    vk::DeviceSize matSize = sizeof(MaterialUBO);
    binding.materialBufs.reserve(MAX_FRAMES_IN_FLIGHT);
    binding.materialMems.reserve(MAX_FRAMES_IN_FLIGHT);
    binding.materialMapped.resize(MAX_FRAMES_IN_FLIGHT);

    MaterialUBO matUbo = toMaterialUBO(binding.geometry->material());

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [mBuf, mMem] = device.createBuffer(matSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        binding.materialMapped[i] = mMem.mapMemory(0, matSize);
        binding.materialBufs.push_back(std::move(mBuf));
        binding.materialMems.push_back(std::move(mMem));

        memcpy(binding.materialMapped[i], &matUbo, sizeof(matUbo));
    }
}

MaterialUBO Object::toMaterialUBO(const Material& mat)
{
    MaterialUBO ubo{};
    ubo.ambient[0] = mat.ambient().r();
    ubo.ambient[1] = mat.ambient().g();
    ubo.ambient[2] = mat.ambient().b();
    ubo.diffuse[0] = mat.diffuse().r();
    ubo.diffuse[1] = mat.diffuse().g();
    ubo.diffuse[2] = mat.diffuse().b();
    ubo.specular[0] = mat.specular().r();
    ubo.specular[1] = mat.specular().g();
    ubo.specular[2] = mat.specular().b();
    ubo.emissive[0] = mat.emissive().r();
    ubo.emissive[1] = mat.emissive().g();
    ubo.emissive[2] = mat.emissive().b();
    ubo.shininess = mat.shininess();
    ubo.ior = mat.ior();
    ubo.transparency = mat.transparency();
    ubo.illum = mat.illum();
    ubo.roughness = mat.roughness();
    ubo.metallic = mat.metallic();
    ubo.sheen = mat.sheen();
    ubo.clearcoat = mat.clearcoat();
    ubo.clearcoatRoughness = mat.clearcoatRoughness();
    ubo.anisotropy = mat.anisotropy();
    ubo.anisotropyRotation = mat.anisotropyRotation();
    return ubo;
}

void Object::createSkinBuffers(const Device& device, GeometryBindings& binding)
{
    vk::DeviceSize skinSize = sizeof(SkinUBO);
    binding.skinBufs.reserve(MAX_FRAMES_IN_FLIGHT);
    binding.skinMems.reserve(MAX_FRAMES_IN_FLIGHT);
    binding.skinMapped.resize(MAX_FRAMES_IN_FLIGHT);

    // Initialise with identity matrices
    SkinUBO skinUbo{};
    for (std::size_t j = 0; j < MAX_JOINTS; ++j)
    {
        skinUbo.joints[j] = Mat4::identity();
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [sBuf, sMem] = device.createBuffer(skinSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        binding.skinMapped[i] = sMem.mapMemory(0, skinSize);
        binding.skinBufs.push_back(std::move(sBuf));
        binding.skinMems.push_back(std::move(sMem));

        memcpy(binding.skinMapped[i], &skinUbo, sizeof(skinUbo));
    }
}

void Object::createDescriptorPool(const Device& device)
{
    auto numGeometries = static_cast<uint32_t>(bindings_.size());
    uint32_t totalSets = numGeometries * MAX_FRAMES_IN_FLIGHT;

    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 3},
        {vk::DescriptorType::eCombinedImageSampler, totalSets},
    }};
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, totalSets,
                                    poolSizes);
    descPool_ = vk::raii::DescriptorPool(device.device(), ci);
}

void Object::createDescriptorSets(const Device& device, const Pipeline& pipeline)
{
    for (auto& binding : bindings_)
    {
        const auto& mat = binding.geometry->material();

        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                     pipeline.descriptorSetLayout());
        vk::DescriptorSetAllocateInfo ai(*descPool_, layouts);
        auto sets = device.device().allocateDescriptorSets(ai);
        binding.descSets.clear();
        binding.descSets.reserve(sets.size());
        for (auto& s : sets)
        {
            binding.descSets.push_back(std::move(s));
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vk::DescriptorBufferInfo uboBufInfo(*uniformBufs_[i], 0, sizeof(UniformBufferObject));
            vk::DescriptorBufferInfo matBufInfo(*binding.materialBufs[i], 0, sizeof(MaterialUBO));
            vk::DescriptorImageInfo texInfo(mat.texture().sampler(), mat.texture().view(),
                                            vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorBufferInfo skinBufInfo(*binding.skinBufs[i], 0, sizeof(SkinUBO));

            std::array<vk::WriteDescriptorSet, 4> writes = {{
                {*binding.descSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &uboBufInfo},
                {*binding.descSets[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &matBufInfo},
                {*binding.descSets[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                 &texInfo},
                {*binding.descSets[i], 3, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &skinBufInfo},
            }};
            device.device().updateDescriptorSets(writes, {});
        }
    }
}

void Object::updateSkin()
{
    if (skin_ != nullptr && !skin_->empty())
    {
        skin_->updateJointMatrices();
    }
}

Mat4 Object::render(const RenderContext& ctx, const Mat4& world)
{
    auto extent = ctx.swapchain.extent();

    // Write shared UBO once
    UniformBufferObject ubo{};
    ubo.model = world;
    ubo.view = Mat4::lookAt(ctx.cameraPosition, ctx.cameraTarget, {0, 1, 0});
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = ctx.cameraPosition.x();
    ubo.cameraPos[1] = ctx.cameraPosition.y();
    ubo.cameraPos[2] = ctx.cameraPosition.z();
    ubo.cameraPos[3] = 0.0f;
    ubo.hasSkin = (skin_ != nullptr && !skin_->empty()) ? 1 : 0;
    memcpy(uniformMapped_[ctx.currentFrame], &ubo, sizeof(ubo));

    // Upload cached joint matrices if skinned
    if (ubo.hasSkin == 1)
    {
        const auto& jointMatrices = skin_->cachedJointMatrices();
        SkinUBO skinUbo{};
        for (std::size_t j = 0; j < jointMatrices.size() && j < MAX_JOINTS; ++j)
        {
            skinUbo.joints[j] = jointMatrices[j];
        }
        for (auto& binding : bindings_)
        {
            memcpy(binding.skinMapped[ctx.currentFrame], &skinUbo, sizeof(skinUbo));
        }
    }

    // Record draw commands for each geometry
    auto cmd = ctx.commandBuffer;
    for (const auto& binding : bindings_)
    {
        cmd.bindVertexBuffers(0, binding.geometry->vertexBuffer(), {vk::DeviceSize{0}});
        cmd.bindIndexBuffer(binding.geometry->indexBuffer(), 0, vk::IndexType::eUint16);
        vk::DescriptorSet ds = *binding.descSets[ctx.currentFrame];
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ctx.pipeline.pipelineLayout(), 0,
                               ds, {});
        cmd.drawIndexed(binding.geometry->indexCount(), 1, 0, 0, 0);
    }

    return world;
}

} // namespace fire_engine
