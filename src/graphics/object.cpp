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
        createMorphBuffers(device, binding);
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

void Object::createMorphBuffers(const Device& device, GeometryBindings& binding)
{
    // Create per-frame morph UBO buffers
    vk::DeviceSize morphUboSize = sizeof(MorphUBO);
    binding.morphUboBufs.reserve(MAX_FRAMES_IN_FLIGHT);
    binding.morphUboMems.reserve(MAX_FRAMES_IN_FLIGHT);
    binding.morphUboMapped.resize(MAX_FRAMES_IN_FLIGHT);

    MorphUBO morphUbo{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [mBuf, mMem] = device.createBuffer(morphUboSize,
                                                vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        binding.morphUboMapped[i] = mMem.mapMemory(0, morphUboSize);
        binding.morphUboBufs.push_back(std::move(mBuf));
        binding.morphUboMems.push_back(std::move(mMem));
        memcpy(binding.morphUboMapped[i], &morphUbo, sizeof(morphUbo));
    }

    // Create SSBO for morph target deltas (static, uploaded once)
    auto numTargets = binding.geometry->morphTargetCount();
    auto numVerts = binding.geometry->vertices().size();

    if (numTargets == 0 || numVerts == 0)
    {
        // Create a minimal dummy SSBO (binding must always be valid)
        vk::DeviceSize dummySize = sizeof(float) * 4;
        auto [sBuf, sMem] = device.createBuffer(dummySize,
                                                vk::BufferUsageFlagBits::eStorageBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        float zeros[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        void* mapped = sMem.mapMemory(0, dummySize);
        memcpy(mapped, zeros, dummySize);
        sMem.unmapMemory();
        binding.morphSsbo = std::move(sBuf);
        binding.morphSsboMem = std::move(sMem);
        return;
    }

    // Layout: [pos_target0_v0..pos_target0_vN, ..., pos_targetM_vN,
    //          norm_target0_v0..norm_target0_vN, ..., norm_targetM_vN]
    // Each entry is a vec4 (xyz + padding)
    std::size_t totalEntries = numTargets * numVerts * 2; // positions + normals
    vk::DeviceSize ssboSize = totalEntries * sizeof(float) * 4;

    auto [sBuf, sMem] = device.createBuffer(ssboSize,
                                            vk::BufferUsageFlagBits::eStorageBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    void* mapped = sMem.mapMemory(0, ssboSize);
    auto* dst = static_cast<float*>(mapped);

    const auto& morphPositions = binding.geometry->morphPositions();
    const auto& morphNormals = binding.geometry->morphNormals();

    // Write position deltas
    for (std::size_t t = 0; t < numTargets; ++t)
    {
        for (std::size_t v = 0; v < numVerts; ++v)
        {
            const auto& pos = (t < morphPositions.size() && v < morphPositions[t].size())
                                  ? morphPositions[t][v]
                                  : Vec3{};
            *dst++ = pos.x();
            *dst++ = pos.y();
            *dst++ = pos.z();
            *dst++ = 0.0f; // padding
        }
    }

    // Write normal deltas
    for (std::size_t t = 0; t < numTargets; ++t)
    {
        for (std::size_t v = 0; v < numVerts; ++v)
        {
            const auto& norm = (t < morphNormals.size() && v < morphNormals[t].size())
                                   ? morphNormals[t][v]
                                   : Vec3{};
            *dst++ = norm.x();
            *dst++ = norm.y();
            *dst++ = norm.z();
            *dst++ = 0.0f; // padding
        }
    }

    sMem.unmapMemory();
    binding.morphSsbo = std::move(sBuf);
    binding.morphSsboMem = std::move(sMem);
}

void Object::createDescriptorPool(const Device& device)
{
    auto numGeometries = static_cast<uint32_t>(bindings_.size());
    uint32_t totalSets = numGeometries * MAX_FRAMES_IN_FLIGHT;

    std::array<vk::DescriptorPoolSize, 3> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 4},
        {vk::DescriptorType::eCombinedImageSampler, totalSets},
        {vk::DescriptorType::eStorageBuffer, totalSets},
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
            vk::DescriptorBufferInfo morphUboBufInfo(*binding.morphUboBufs[i], 0,
                                                     sizeof(MorphUBO));

            vk::DeviceSize ssboSize = binding.geometry->morphTargetCount() > 0
                                          ? binding.geometry->morphTargetCount() *
                                                binding.geometry->vertices().size() * 2 *
                                                sizeof(float) * 4
                                          : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboBufInfo(*binding.morphSsbo, 0, ssboSize);

            std::array<vk::WriteDescriptorSet, 6> writes = {{
                {*binding.descSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &uboBufInfo},
                {*binding.descSets[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &matBufInfo},
                {*binding.descSets[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                 &texInfo},
                {*binding.descSets[i], 3, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &skinBufInfo},
                {*binding.descSets[i], 4, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &morphUboBufInfo},
                {*binding.descSets[i], 5, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                 &morphSsboBufInfo},
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

    // Upload morph weights
    for (auto& binding : bindings_)
    {
        auto numTargets = binding.geometry->morphTargetCount();
        MorphUBO morphUbo{};
        if (numTargets > 0 && !morphWeights_.empty())
        {
            morphUbo.hasMorph = 1;
            morphUbo.morphTargetCount = static_cast<int>(numTargets);
            morphUbo.vertexCount = static_cast<int>(binding.geometry->vertices().size());
            for (std::size_t w = 0;
                 w < morphWeights_.size() && w < static_cast<std::size_t>(MAX_MORPH_TARGETS); ++w)
            {
                morphUbo.weights[w] = morphWeights_[w];
            }
        }
        memcpy(binding.morphUboMapped[ctx.currentFrame], &morphUbo, sizeof(morphUbo));
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
