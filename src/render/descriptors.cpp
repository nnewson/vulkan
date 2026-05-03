#include <fire_engine/render/descriptors.hpp>

#include <fire_engine/render/device.hpp>
#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/resources.hpp>
#include <fire_engine/render/ubo.hpp>

namespace fire_engine
{

vk::DescriptorBufferInfo Descriptors::makeDescriptorBufferInfo(vk::Buffer buffer,
                                                               vk::DeviceSize range)
{
    return vk::DescriptorBufferInfo{
        .buffer = buffer,
        .offset = 0,
        .range = range,
    };
}

vk::DescriptorImageInfo Descriptors::makeDescriptorImageInfo(vk::Sampler sampler,
                                                             vk::ImageView imageView,
                                                             vk::ImageLayout imageLayout)
{
    return vk::DescriptorImageInfo{
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = imageLayout,
    };
}

Descriptors::Descriptors(const Device& device, const Pipeline& pipeline, const Resources& resources)
    : device_{&device},
      pipeline_{&pipeline},
      resources_{&resources}
{
}

Descriptors::DescriptorPoolEntry&
Descriptors::createDescriptorPool(std::span<const vk::DescriptorPoolSize> poolSizes,
                                  uint32_t maxSets)
{
    vk::DescriptorPoolCreateInfo ci{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    auto& poolEntry = descriptorPools_.emplace_back();
    poolEntry.pool = vk::raii::DescriptorPool(device_->device(), ci);
    return poolEntry;
}

std::vector<vk::raii::DescriptorSet>
Descriptors::allocateDescriptorSets(vk::DescriptorPool pool, vk::DescriptorSetLayout layout,
                                    uint32_t count) const
{
    std::vector<vk::DescriptorSetLayout> layouts(count, layout);
    vk::DescriptorSetAllocateInfo ai{
        .descriptorPool = pool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    return device_->device().allocateDescriptorSets(ai);
}

DescriptorSetHandle Descriptors::registerDescriptorSet(vk::DescriptorSet set)
{
    auto dsHandle = static_cast<uint32_t>(descriptorSetTable_.size());
    descriptorSetTable_.push_back(set);
    return DescriptorSetHandle{dsHandle};
}

void Descriptors::retainDescriptorSets(DescriptorPoolEntry& poolEntry,
                                       std::vector<vk::raii::DescriptorSet>& sets)
{
    for (auto& s : sets)
    {
        poolEntry.sets.push_back(std::move(s));
    }
}

ObjectDescriptorResult Descriptors::createObjectDescriptors(const ObjectDescriptorRequest& req)
{
    auto numGeometries = static_cast<uint32_t>(req.geometries.size());
    uint32_t totalSets = numGeometries * MAX_FRAMES_IN_FLIGHT;

    std::array<vk::DescriptorPoolSize, 3> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 5},
        {vk::DescriptorType::eCombinedImageSampler, totalSets * 11},
        {vk::DescriptorType::eStorageBuffer, totalSets},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, totalSets);

    ObjectDescriptorResult result;
    result.descSets.resize(numGeometries);

    for (uint32_t g = 0; g < numGeometries; ++g)
    {
        const auto& geo = req.geometries[g];
        auto sets = allocateDescriptorSets(*poolEntry.pool, pipeline_->descriptorSetLayout(),
                                           MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vk::DescriptorBufferInfo uboBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(req.uniformBufs[i]), sizeof(UniformBufferObject));
            vk::DescriptorBufferInfo matBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(geo.materialBufs[i]), sizeof(MaterialUBO));
            vk::DescriptorImageInfo texInfo = makeDescriptorImageInfo(
                resources_->vulkanSampler(geo.texture), resources_->vulkanImageView(geo.texture),
                vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorBufferInfo skinBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(geo.skinBufs[i]), sizeof(SkinUBO));
            vk::DescriptorBufferInfo morphUboBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(geo.morphUboBufs[i]), sizeof(MorphUBO));

            vk::DeviceSize ssboSize = geo.morphSsboSize > 0
                                          ? static_cast<vk::DeviceSize>(geo.morphSsboSize)
                                          : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboBufInfo =
                makeDescriptorBufferInfo(resources_->vulkanBuffer(geo.morphSsbo), ssboSize);

            vk::DescriptorImageInfo emissiveTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(geo.emissiveTexture),
                                        resources_->vulkanImageView(geo.emissiveTexture),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo normalTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(geo.normalTexture),
                                        resources_->vulkanImageView(geo.normalTexture),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo mrTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(geo.metallicRoughnessTexture),
                                        resources_->vulkanImageView(geo.metallicRoughnessTexture),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo occTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(geo.occlusionTexture),
                                        resources_->vulkanImageView(geo.occlusionTexture),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo transTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(geo.transmissionTexture),
                                        resources_->vulkanImageView(geo.transmissionTexture),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorBufferInfo lightBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(req.lightBufs[i]), sizeof(LightUBO));
            vk::DescriptorImageInfo shadowTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(req.shadowMap),
                                        resources_->vulkanImageView(req.shadowMap),
                                        vk::ImageLayout::eDepthStencilReadOnlyOptimal);
            vk::DescriptorImageInfo shadowDepthTexInfo =
                makeDescriptorImageInfo(resources_->vulkanShadowSamplerLinear(req.shadowMap),
                                        resources_->vulkanImageView(req.shadowMap),
                                        vk::ImageLayout::eDepthStencilReadOnlyOptimal);
            vk::DescriptorImageInfo irradianceTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(req.irradianceMap),
                                        resources_->vulkanImageView(req.irradianceMap),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo prefilteredTexInfo =
                makeDescriptorImageInfo(resources_->vulkanSampler(req.prefilteredMap),
                                        resources_->vulkanImageView(req.prefilteredMap),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::DescriptorImageInfo brdfLutTexInfo = makeDescriptorImageInfo(
                resources_->vulkanSampler(req.brdfLut), resources_->vulkanImageView(req.brdfLut),
                vk::ImageLayout::eShaderReadOnlyOptimal);

            std::array<vk::WriteDescriptorSet, 17> writes = {{
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 0,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &uboBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 1,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &matBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 2,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &texInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 3,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &skinBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 4,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &morphUboBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 5,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eStorageBuffer,
                                       .pBufferInfo = &morphSsboBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 6,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &emissiveTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 7,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &normalTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 8,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &mrTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 9,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &occTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 10,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &shadowTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 11,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &lightBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 12,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &irradianceTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 13,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &prefilteredTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 14,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &brdfLutTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 15,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &shadowDepthTexInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 16,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                       .pImageInfo = &transTexInfo},
            }};
            device_->device().updateDescriptorSets(writes, {});

            result.descSets[g][i] = registerDescriptorSet(*sets[i]);
        }

        retainDescriptorSets(poolEntry, sets);
    }

    return result;
}

ShadowDescriptorResult Descriptors::createShadowDescriptors(const ShadowDescriptorRequest& req)
{
    auto numGeometries = static_cast<uint32_t>(req.geometries.size());
    uint32_t totalSets = numGeometries * MAX_FRAMES_IN_FLIGHT;

    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 3},
        {vk::DescriptorType::eStorageBuffer, totalSets},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, totalSets);

    ShadowDescriptorResult result;
    result.descSets.resize(numGeometries);

    for (uint32_t g = 0; g < numGeometries; ++g)
    {
        const auto& geo = req.geometries[g];
        auto sets =
            allocateDescriptorSets(*poolEntry.pool, shadowDescLayout_, MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vk::DescriptorBufferInfo shadowUboInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(geo.shadowUboBufs[i]), sizeof(ShadowUBO));
            vk::DescriptorBufferInfo skinBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(geo.skinBufs[i]), sizeof(SkinUBO));
            vk::DescriptorBufferInfo morphUboBufInfo = makeDescriptorBufferInfo(
                resources_->vulkanBuffer(geo.morphUboBufs[i]), sizeof(MorphUBO));
            vk::DeviceSize ssboSize = geo.morphSsboSize > 0
                                          ? static_cast<vk::DeviceSize>(geo.morphSsboSize)
                                          : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboInfo =
                makeDescriptorBufferInfo(resources_->vulkanBuffer(geo.morphSsbo), ssboSize);

            std::array<vk::WriteDescriptorSet, 4> writes = {{
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 0,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &shadowUboInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 1,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &skinBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 2,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                       .pBufferInfo = &morphUboBufInfo},
                vk::WriteDescriptorSet{.dstSet = *sets[i],
                                       .dstBinding = 3,
                                       .descriptorCount = 1,
                                       .descriptorType = vk::DescriptorType::eStorageBuffer,
                                       .pBufferInfo = &morphSsboInfo},
            }};
            device_->device().updateDescriptorSets(writes, {});

            result.descSets[g][i] = registerDescriptorSet(*sets[i]);
        }

        retainDescriptorSets(poolEntry, sets);
    }

    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Descriptors::createSingleUboDescriptors(vk::DescriptorSetLayout layout, const MappedBufferSet& ubo,
                                        vk::DeviceSize uboSize)
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo bufInfo =
            makeDescriptorBufferInfo(resources_->vulkanBuffer(ubo.buffers[i]), uboSize);
        vk::WriteDescriptorSet write{
            .dstSet = *sets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufInfo,
        };
        device_->device().updateDescriptorSets(write, {});

        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);
    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Descriptors::createUboImageSamplerDescriptors(vk::DescriptorSetLayout layout,
                                              const MappedBufferSet& ubo, vk::DeviceSize uboSize,
                                              TextureHandle texture)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
        {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo bufInfo =
            makeDescriptorBufferInfo(resources_->vulkanBuffer(ubo.buffers[i]), uboSize);
        vk::DescriptorImageInfo texInfo = makeDescriptorImageInfo(
            resources_->vulkanSampler(texture), resources_->vulkanImageView(texture),
            vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 2> writes = {{
            vk::WriteDescriptorSet{.dstSet = *sets[i],
                                   .dstBinding = 0,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eUniformBuffer,
                                   .pBufferInfo = &bufInfo},
            vk::WriteDescriptorSet{.dstSet = *sets[i],
                                   .dstBinding = 1,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                   .pImageInfo = &texInfo},
        }};
        device_->device().updateDescriptorSets(writes, {});
        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);
    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> Descriptors::createSkyboxDescriptors(
    vk::DescriptorSetLayout layout, const MappedBufferSet& skyboxUbo, vk::DeviceSize skyboxUboSize,
    TextureHandle texture, const MappedBufferSet& lightUbo, vk::DeviceSize lightUboSize)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT * 2},
        {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo skyboxBufInfo =
            makeDescriptorBufferInfo(resources_->vulkanBuffer(skyboxUbo.buffers[i]), skyboxUboSize);
        vk::DescriptorBufferInfo lightBufInfo =
            makeDescriptorBufferInfo(resources_->vulkanBuffer(lightUbo.buffers[i]), lightUboSize);
        vk::DescriptorImageInfo texInfo = makeDescriptorImageInfo(
            resources_->vulkanSampler(texture), resources_->vulkanImageView(texture),
            vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 3> writes = {{
            vk::WriteDescriptorSet{.dstSet = *sets[i],
                                   .dstBinding = 0,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eUniformBuffer,
                                   .pBufferInfo = &skyboxBufInfo},
            vk::WriteDescriptorSet{.dstSet = *sets[i],
                                   .dstBinding = 1,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                   .pImageInfo = &texInfo},
            vk::WriteDescriptorSet{.dstSet = *sets[i],
                                   .dstBinding = 2,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eUniformBuffer,
                                   .pBufferInfo = &lightBufInfo},
        }};
        device_->device().updateDescriptorSets(writes, {});
        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);
    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Descriptors::createSingleImageSamplerDescriptors(vk::DescriptorSetLayout layout,
                                                 TextureHandle texture)
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);
    updateSingleImageSamplerDescriptors(result, texture);
    return result;
}

void Descriptors::updateSingleImageSamplerDescriptors(
    const std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>& sets, TextureHandle texture)
{
    vk::DescriptorImageInfo imgInfo = makeDescriptorImageInfo(
        resources_->vulkanSampler(texture), resources_->vulkanImageView(texture),
        vk::ImageLayout::eShaderReadOnlyOptimal);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::WriteDescriptorSet write{
            .dstSet = descriptorSetTable_[static_cast<uint32_t>(sets[i])],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imgInfo,
        };
        device_->device().updateDescriptorSets(write, {});
    }
}

DescriptorSetHandle Descriptors::createImageViewDescriptor(vk::DescriptorSetLayout layout,
                                                           vk::ImageView view, vk::Sampler sampler)
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {vk::DescriptorType::eCombinedImageSampler, 1},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, 1);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, 1);

    vk::DescriptorImageInfo imgInfo =
        makeDescriptorImageInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet write{
        .dstSet = *sets[0],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imgInfo,
    };
    device_->device().updateDescriptorSets(write, {});

    auto handle = registerDescriptorSet(*sets[0]);
    retainDescriptorSets(poolEntry, sets);
    return handle;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Descriptors::createPostProcessDescriptors(vk::DescriptorSetLayout layout, TextureHandle hdrTarget,
                                          TextureHandle bloomChain)
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT * 2},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        result[i] = registerDescriptorSet(*sets[i]);
    }
    retainDescriptorSets(poolEntry, sets);

    updatePostProcessDescriptors(result, hdrTarget, bloomChain);
    return result;
}

void Descriptors::updatePostProcessDescriptors(
    const std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>& sets, TextureHandle hdrTarget,
    TextureHandle bloomChain)
{
    vk::ImageView bloomMip0 = resources_->vulkanBloomMipView(bloomChain, 0);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorImageInfo hdrInfo = makeDescriptorImageInfo(
            resources_->vulkanSampler(hdrTarget), resources_->vulkanImageView(hdrTarget),
            vk::ImageLayout::eShaderReadOnlyOptimal);
        vk::DescriptorImageInfo bloomInfo =
            makeDescriptorImageInfo(resources_->vulkanSampler(bloomChain), bloomMip0,
                                    vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 2> writes = {{
            vk::WriteDescriptorSet{.dstSet = descriptorSetTable_[static_cast<uint32_t>(sets[i])],
                                   .dstBinding = 0,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                   .pImageInfo = &hdrInfo},
            vk::WriteDescriptorSet{.dstSet = descriptorSetTable_[static_cast<uint32_t>(sets[i])],
                                   .dstBinding = 1,
                                   .descriptorCount = 1,
                                   .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                   .pImageInfo = &bloomInfo},
        }};
        device_->device().updateDescriptorSets(writes, {});
    }
}

vk::DescriptorSet Descriptors::vulkanDescriptorSet(DescriptorSetHandle handle) const noexcept
{
    return descriptorSetTable_[static_cast<uint32_t>(handle)];
}

} // namespace fire_engine
