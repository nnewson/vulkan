#include <fire_engine/render/resources.hpp>

#include <cstring>

#include <fire_engine/graphics/image.hpp>
#include <fire_engine/graphics/vertex.hpp>
#include <fire_engine/render/device.hpp>
#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/ubo.hpp>

namespace fire_engine
{

Resources::Resources(const Device& device, const Pipeline& pipeline)
    : device_(&device),
      pipeline_(&pipeline)
{
    vk::CommandPoolCreateInfo poolCi(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                     device.graphicsFamily());
    cmdPool_ = vk::raii::CommandPool(device.device(), poolCi);
}

// --- Buffer helpers ---

BufferHandle Resources::storeBuffer(vk::raii::Buffer buf, vk::raii::DeviceMemory mem)
{
    auto id = static_cast<uint32_t>(buffers_.size());
    buffers_.push_back({std::move(buf), std::move(mem)});
    return BufferHandle{id};
}

// --- Buffer creation ---

BufferHandle Resources::createVertexBuffer(std::span<const Vertex> vertices)
{
    vk::DeviceSize size = vertices.size_bytes();
    auto [buf, mem] = device_->createBuffer(size, vk::BufferUsageFlagBits::eVertexBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = mem.mapMemory(0, size);
    std::memcpy(data, vertices.data(), size);
    mem.unmapMemory();
    return storeBuffer(std::move(buf), std::move(mem));
}

BufferHandle Resources::createIndexBuffer(std::span<const uint16_t> indices)
{
    vk::DeviceSize size = indices.size_bytes();
    auto [buf, mem] = device_->createBuffer(size, vk::BufferUsageFlagBits::eIndexBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = mem.mapMemory(0, size);
    std::memcpy(data, indices.data(), size);
    mem.unmapMemory();
    return storeBuffer(std::move(buf), std::move(mem));
}

// --- Texture creation ---

static vk::SamplerAddressMode toVkAddressMode(WrapMode mode)
{
    switch (mode)
    {
        case WrapMode::MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
        case WrapMode::ClampToEdge: return vk::SamplerAddressMode::eClampToEdge;
        default: return vk::SamplerAddressMode::eRepeat;
    }
}

static vk::Filter toVkFilter(FilterMode mode)
{
    switch (mode)
    {
        case FilterMode::Nearest: return vk::Filter::eNearest;
        default: return vk::Filter::eLinear;
    }
}

TextureHandle Resources::createTexture(const Image& image, const SamplerSettings& sampler)
{
    return createTexture(image.data(), image.width(), image.height(), sampler);
}

TextureHandle Resources::createTexture(const uint8_t* pixels, int width, int height,
                                       const SamplerSettings& sampler)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();

    vk::DeviceSize imageSize =
        static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4;

    // Create staging buffer
    auto [stagingBuf, stagingMem] =
        device_->createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible |
                                  vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = stagingMem.mapMemory(0, imageSize);
    std::memcpy(data, pixels, imageSize);
    stagingMem.unmapMemory();

    // Create image
    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    // Transition and copy via one-shot command buffer
    vk::CommandBufferAllocateInfo cmdAi(*cmdPool_, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device_->device().allocateCommandBuffers(cmdAi);
    auto& cmd = cmds[0];

    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Transition: undefined -> transfer dst
    vk::ImageMemoryBarrier toTransfer(
        {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        *entry.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {}, toTransfer);

    // Copy buffer to image
    vk::BufferImageCopy region(
        0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), {0, 0, 0},
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1});
    cmd.copyBufferToImage(*stagingBuf, *entry.image, vk::ImageLayout::eTransferDstOptimal, region);

    // Transition: transfer dst -> shader read
    vk::ImageMemoryBarrier toShader(
        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *entry.image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);

    cmd.end();

    vk::CommandBuffer rawCmd = *cmd;
    vk::SubmitInfo submitInfo({}, {}, rawCmd);
    device_->graphicsQueue().submit(submitInfo);
    device_->graphicsQueue().waitIdle();

    // Create image view
    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    // Create sampler
    auto props = device_->physicalDevice().getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, toVkFilter(sampler.magFilter), toVkFilter(sampler.minFilter),
        vk::SamplerMipmapMode::eLinear, toVkAddressMode(sampler.wrapS),
        toVkAddressMode(sampler.wrapT), toVkAddressMode(sampler.wrapS), 0.0f, vk::True,
        props.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways, 0.0f, 0.0f,
        vk::BorderColor::eIntOpaqueBlack, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

// --- Shadow map ---

TextureHandle Resources::createShadowMap(uint32_t extent)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();

    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, vk::Format::eD32Sfloat, vk::Extent3D(extent, extent, 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, vk::Format::eD32Sfloat, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    // Comparison sampler for sampler2DShadow: hardware PCF via eLess compare,
    // clamp-to-border with white border so off-map samples return "lit".
    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder, 0.0f, vk::False, 1.0f, vk::True,
        vk::CompareOp::eLess, 0.0f, 1.0f, vk::BorderColor::eFloatOpaqueWhite, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

// --- Mapped buffer sets ---

Resources::MappedBufferSet Resources::createMappedUniformBuffers(std::size_t size)
{
    MappedBufferSet result;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [buf, mem] =
            device_->createBuffer(static_cast<vk::DeviceSize>(size),
                                  vk::BufferUsageFlagBits::eUniformBuffer,
                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                      vk::MemoryPropertyFlagBits::eHostCoherent);
        result.mapped[i] = mem.mapMemory(0, static_cast<vk::DeviceSize>(size));
        result.buffers[i] = storeBuffer(std::move(buf), std::move(mem));
    }
    return result;
}

Resources::MappedBufferSet
Resources::createMappedStorageBuffer(std::size_t size, const void* initialData)
{
    MappedBufferSet result;
    // Storage buffers are shared across frames — create a single buffer,
    // but fill both slots so callers can index by frame uniformly
    auto [buf, mem] =
        device_->createBuffer(static_cast<vk::DeviceSize>(size),
                              vk::BufferUsageFlagBits::eStorageBuffer,
                              vk::MemoryPropertyFlagBits::eHostVisible |
                                  vk::MemoryPropertyFlagBits::eHostCoherent);
    if (initialData != nullptr)
    {
        void* mapped = mem.mapMemory(0, static_cast<vk::DeviceSize>(size));
        std::memcpy(mapped, initialData, size);
        mem.unmapMemory();
    }
    auto handle = storeBuffer(std::move(buf), std::move(mem));
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        result.buffers[i] = handle;
        result.mapped[i] = nullptr;
    }
    return result;
}

// --- Descriptor sets ---

Resources::ObjectDescriptorResult
Resources::createObjectDescriptors(const ObjectDescriptorRequest& req)
{
    auto numGeometries = static_cast<uint32_t>(req.geometries.size());
    uint32_t totalSets = numGeometries * MAX_FRAMES_IN_FLIGHT;

    // Create descriptor pool
    std::array<vk::DescriptorPoolSize, 3> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 5},
        {vk::DescriptorType::eCombinedImageSampler, totalSets * 5},
        {vk::DescriptorType::eStorageBuffer, totalSets},
    }};
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, totalSets,
                                    poolSizes);
    auto& poolEntry = descriptorPools_.emplace_back();
    poolEntry.pool = vk::raii::DescriptorPool(device_->device(), ci);

    ObjectDescriptorResult result;
    result.descSets.resize(numGeometries);

    for (uint32_t g = 0; g < numGeometries; ++g)
    {
        const auto& geo = req.geometries[g];

        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                     pipeline_->descriptorSetLayout());
        vk::DescriptorSetAllocateInfo ai(*poolEntry.pool, layouts);
        auto sets = device_->device().allocateDescriptorSets(ai);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vk::DescriptorBufferInfo uboBufInfo(
                *buffers_[static_cast<uint32_t>(req.uniformBufs[i])].buffer, 0,
                sizeof(UniformBufferObject));
            vk::DescriptorBufferInfo matBufInfo(
                *buffers_[static_cast<uint32_t>(geo.materialBufs[i])].buffer, 0,
                sizeof(MaterialUBO));

            auto texIdx = static_cast<uint32_t>(geo.texture);
            vk::DescriptorImageInfo texInfo(
                *textures_[texIdx].sampler, *textures_[texIdx].view,
                vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::DescriptorBufferInfo skinBufInfo(
                *buffers_[static_cast<uint32_t>(geo.skinBufs[i])].buffer, 0, sizeof(SkinUBO));
            vk::DescriptorBufferInfo morphUboBufInfo(
                *buffers_[static_cast<uint32_t>(geo.morphUboBufs[i])].buffer, 0, sizeof(MorphUBO));

            vk::DeviceSize ssboSize =
                geo.morphSsboSize > 0 ? static_cast<vk::DeviceSize>(geo.morphSsboSize)
                                      : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboBufInfo(
                *buffers_[static_cast<uint32_t>(geo.morphSsbo)].buffer, 0, ssboSize);

            auto emissiveTexIdx = static_cast<uint32_t>(geo.emissiveTexture);
            vk::DescriptorImageInfo emissiveTexInfo(
                *textures_[emissiveTexIdx].sampler, *textures_[emissiveTexIdx].view,
                vk::ImageLayout::eShaderReadOnlyOptimal);

            auto normalTexIdx = static_cast<uint32_t>(geo.normalTexture);
            vk::DescriptorImageInfo normalTexInfo(
                *textures_[normalTexIdx].sampler, *textures_[normalTexIdx].view,
                vk::ImageLayout::eShaderReadOnlyOptimal);

            auto mrTexIdx = static_cast<uint32_t>(geo.metallicRoughnessTexture);
            vk::DescriptorImageInfo mrTexInfo(
                *textures_[mrTexIdx].sampler, *textures_[mrTexIdx].view,
                vk::ImageLayout::eShaderReadOnlyOptimal);

            auto occTexIdx = static_cast<uint32_t>(geo.occlusionTexture);
            vk::DescriptorImageInfo occTexInfo(
                *textures_[occTexIdx].sampler, *textures_[occTexIdx].view,
                vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::DescriptorBufferInfo lightBufInfo(
                *buffers_[static_cast<uint32_t>(req.lightBufs[i])].buffer, 0, sizeof(LightUBO));

            std::array<vk::WriteDescriptorSet, 11> writes = {{
                {*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboBufInfo},
                {*sets[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &matBufInfo},
                {*sets[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
                {*sets[i], 3, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &skinBufInfo},
                {*sets[i], 4, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &morphUboBufInfo},
                {*sets[i], 5, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                 &morphSsboBufInfo},
                {*sets[i], 6, 0, 1, vk::DescriptorType::eCombinedImageSampler, &emissiveTexInfo},
                {*sets[i], 7, 0, 1, vk::DescriptorType::eCombinedImageSampler, &normalTexInfo},
                {*sets[i], 8, 0, 1, vk::DescriptorType::eCombinedImageSampler, &mrTexInfo},
                {*sets[i], 9, 0, 1, vk::DescriptorType::eCombinedImageSampler, &occTexInfo},
                {*sets[i], 11, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &lightBufInfo},
            }};
            device_->device().updateDescriptorSets(writes, {});

            auto dsHandle = static_cast<uint32_t>(descriptorSetTable_.size());
            descriptorSetTable_.push_back(*sets[i]);
            result.descSets[g][i] = DescriptorSetHandle{dsHandle};
        }

        // Store RAII ownership
        for (auto& s : sets)
        {
            poolEntry.sets.push_back(std::move(s));
        }
    }

    return result;
}

Resources::ShadowDescriptorResult
Resources::createShadowDescriptors(const ShadowDescriptorRequest& req)
{
    auto numGeometries = static_cast<uint32_t>(req.geometries.size());
    uint32_t totalSets = numGeometries * MAX_FRAMES_IN_FLIGHT;

    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 3},
        {vk::DescriptorType::eStorageBuffer, totalSets},
    }};
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, totalSets,
                                    poolSizes);
    auto& poolEntry = descriptorPools_.emplace_back();
    poolEntry.pool = vk::raii::DescriptorPool(device_->device(), ci);

    ShadowDescriptorResult result;
    result.descSets.resize(numGeometries);

    for (uint32_t g = 0; g < numGeometries; ++g)
    {
        const auto& geo = req.geometries[g];

        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, shadowDescLayout_);
        vk::DescriptorSetAllocateInfo ai(*poolEntry.pool, layouts);
        auto sets = device_->device().allocateDescriptorSets(ai);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vk::DescriptorBufferInfo shadowUboInfo(
                *buffers_[static_cast<uint32_t>(geo.shadowUboBufs[i])].buffer, 0,
                sizeof(ShadowUBO));
            vk::DescriptorBufferInfo skinBufInfo(
                *buffers_[static_cast<uint32_t>(geo.skinBufs[i])].buffer, 0, sizeof(SkinUBO));
            vk::DescriptorBufferInfo morphUboBufInfo(
                *buffers_[static_cast<uint32_t>(geo.morphUboBufs[i])].buffer, 0, sizeof(MorphUBO));

            vk::DeviceSize ssboSize =
                geo.morphSsboSize > 0 ? static_cast<vk::DeviceSize>(geo.morphSsboSize)
                                      : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboInfo(
                *buffers_[static_cast<uint32_t>(geo.morphSsbo)].buffer, 0, ssboSize);

            std::array<vk::WriteDescriptorSet, 4> writes = {{
                {*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &shadowUboInfo},
                {*sets[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &skinBufInfo},
                {*sets[i], 2, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                 &morphUboBufInfo},
                {*sets[i], 3, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr,
                 &morphSsboInfo},
            }};
            device_->device().updateDescriptorSets(writes, {});

            auto dsHandle = static_cast<uint32_t>(descriptorSetTable_.size());
            descriptorSetTable_.push_back(*sets[i]);
            result.descSets[g][i] = DescriptorSetHandle{dsHandle};
        }

        for (auto& s : sets)
        {
            poolEntry.sets.push_back(std::move(s));
        }
    }

    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Resources::createSingleUboDescriptors(vk::DescriptorSetLayout layout, const MappedBufferSet& ubo,
                                      vk::DeviceSize uboSize)
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
    }};
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                    MAX_FRAMES_IN_FLIGHT, poolSizes);
    auto& poolEntry = descriptorPools_.emplace_back();
    poolEntry.pool = vk::raii::DescriptorPool(device_->device(), ci);

    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
    vk::DescriptorSetAllocateInfo ai(*poolEntry.pool, layouts);
    auto sets = device_->device().allocateDescriptorSets(ai);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo bufInfo(
            *buffers_[static_cast<uint32_t>(ubo.buffers[i])].buffer, 0, uboSize);
        vk::WriteDescriptorSet write(*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                     &bufInfo);
        device_->device().updateDescriptorSets(write, {});

        auto dsHandle = static_cast<uint32_t>(descriptorSetTable_.size());
        descriptorSetTable_.push_back(*sets[i]);
        result[i] = DescriptorSetHandle{dsHandle};
    }

    for (auto& s : sets)
    {
        poolEntry.sets.push_back(std::move(s));
    }

    return result;
}

// --- Pipeline registry ---

PipelineHandle Resources::registerPipeline(vk::Pipeline pipeline, vk::PipelineLayout layout)
{
    auto id = static_cast<uint32_t>(pipelines_.size());
    pipelines_.push_back({pipeline, layout});
    return PipelineHandle{id};
}

// --- Vulkan accessors ---

vk::Buffer Resources::vulkanBuffer(BufferHandle handle) const noexcept
{
    return *buffers_[static_cast<uint32_t>(handle)].buffer;
}

vk::ImageView Resources::vulkanImageView(TextureHandle handle) const noexcept
{
    return *textures_[static_cast<uint32_t>(handle)].view;
}

vk::Sampler Resources::vulkanSampler(TextureHandle handle) const noexcept
{
    return *textures_[static_cast<uint32_t>(handle)].sampler;
}

vk::DescriptorSet Resources::vulkanDescriptorSet(DescriptorSetHandle handle) const noexcept
{
    return descriptorSetTable_[static_cast<uint32_t>(handle)];
}

vk::Pipeline Resources::vulkanPipeline(PipelineHandle handle) const noexcept
{
    return pipelines_[static_cast<uint32_t>(handle)].pipeline;
}

vk::PipelineLayout Resources::vulkanPipelineLayout(PipelineHandle handle) const noexcept
{
    return pipelines_[static_cast<uint32_t>(handle)].layout;
}

} // namespace fire_engine
