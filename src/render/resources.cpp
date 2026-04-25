#include <fire_engine/render/resources.hpp>

#include <algorithm>
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

BufferHandle Resources::createIndexBuffer(std::span<const uint32_t> indices)
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
    case WrapMode::MirroredRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case WrapMode::ClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    default:
        return vk::SamplerAddressMode::eRepeat;
    }
}

static vk::Filter toVkFilter(FilterMode mode)
{
    switch (mode)
    {
    case FilterMode::Nearest:
        return vk::Filter::eNearest;
    default:
        return vk::Filter::eLinear;
    }
}

static vk::Format toVkTextureFormat(TextureEncoding encoding)
{
    return encoding == TextureEncoding::Srgb ? vk::Format::eR8G8B8A8Srgb
                                             : vk::Format::eR8G8B8A8Unorm;
}

static std::size_t fallbackTextureIndex(Resources::FallbackTextureKind kind)
{
    return static_cast<std::size_t>(kind);
}

void Resources::createCubemapFaceViews(const Device& device, TextureEntry& entry)
{
    entry.faceViews.clear();
    entry.faceViews.reserve(static_cast<std::size_t>(entry.mipLevels) * 6);

    for (uint32_t mipLevel = 0; mipLevel < entry.mipLevels; ++mipLevel)
    {
        for (uint32_t face = 0; face < 6; ++face)
        {
            vk::ImageViewCreateInfo faceViewCi(
                {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mipLevel, 1, face, 1));
            entry.faceViews.emplace_back(device.device(), faceViewCi);
        }
    }
}

TextureHandle Resources::createTexture(const Image& image, const SamplerSettings& sampler,
                                       TextureEncoding encoding)
{
    if (image.pixelType() == ImagePixelType::Float32)
    {
        return createTexture(image.dataf(), image.width(), image.height(), sampler);
    }

    return createTexture(image.data(), image.width(), image.height(), sampler, encoding);
}

TextureHandle Resources::createTexture(const uint8_t* pixels, int width, int height,
                                       const SamplerSettings& sampler, TextureEncoding encoding)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = toVkTextureFormat(encoding);

    vk::DeviceSize imageSize =
        static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4;

    auto [stagingBuf, stagingMem] = device_->createBuffer(
        imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = stagingMem.mapMemory(0, imageSize);
    std::memcpy(data, pixels, static_cast<std::size_t>(imageSize));
    stagingMem.unmapMemory();

    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, entry.format,
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

    vk::CommandBufferAllocateInfo cmdAi(*cmdPool_, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device_->device().allocateCommandBuffers(cmdAi);
    auto& cmd = cmds[0];

    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::ImageMemoryBarrier toTransfer(
        {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        *entry.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {}, toTransfer);

    vk::BufferImageCopy region(
        0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), {0, 0, 0},
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1});
    cmd.copyBufferToImage(*stagingBuf, *entry.image, vk::ImageLayout::eTransferDstOptimal, region);

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

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

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

TextureHandle Resources::createTexture(const float* pixels, int width, int height,
                                       const SamplerSettings& sampler)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = vk::Format::eR32G32B32A32Sfloat;
    entry.mipLevels = 1;

    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(width) *
                               static_cast<vk::DeviceSize>(height) * 4 * sizeof(float);

    auto [stagingBuf, stagingMem] = device_->createBuffer(
        imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = stagingMem.mapMemory(0, imageSize);
    std::memcpy(data, pixels, static_cast<std::size_t>(imageSize));
    stagingMem.unmapMemory();

    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, entry.format,
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

    vk::CommandBufferAllocateInfo cmdAi(*cmdPool_, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device_->device().allocateCommandBuffers(cmdAi);
    auto& cmd = cmds[0];

    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::ImageMemoryBarrier toTransfer(
        {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        *entry.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {}, toTransfer);

    vk::BufferImageCopy region(
        0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), {0, 0, 0},
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1});
    cmd.copyBufferToImage(*stagingBuf, *entry.image, vk::ImageLayout::eTransferDstOptimal, region);

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

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    auto props = device_->physicalDevice().getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, toVkFilter(sampler.magFilter), toVkFilter(sampler.minFilter),
        vk::SamplerMipmapMode::eLinear, toVkAddressMode(sampler.wrapS),
        toVkAddressMode(sampler.wrapT), toVkAddressMode(sampler.wrapS), 0.0f, vk::True,
        props.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways, 0.0f, 0.0f,
        vk::BorderColor::eFloatOpaqueBlack, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

TextureHandle Resources::createCubemapTexture(const float* pixels, uint32_t faceExtent,
                                              const SamplerSettings& sampler)
{
    return createCubemapTexture(pixels, faceExtent, 1, sampler);
}

TextureHandle Resources::createCubemapTexture(const float* pixels, uint32_t faceExtent,
                                              uint32_t mipLevels, const SamplerSettings& sampler)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = vk::Format::eR32G32B32A32Sfloat;
    entry.mipLevels = mipLevels;

    vk::DeviceSize imageSize = 0;
    uint32_t levelExtent = faceExtent;
    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        vk::DeviceSize faceSize =
            static_cast<vk::DeviceSize>(levelExtent) * levelExtent * 4 * sizeof(float);
        imageSize += faceSize * 6;
        levelExtent = std::max(1u, levelExtent / 2);
    }

    auto [stagingBuf, stagingMem] = device_->createBuffer(
        imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = stagingMem.mapMemory(0, imageSize);
    std::memcpy(data, pixels, static_cast<std::size_t>(imageSize));
    stagingMem.unmapMemory();

    vk::ImageCreateInfo imgCi(vk::ImageCreateFlagBits::eCubeCompatible, vk::ImageType::e2D,
                              entry.format, vk::Extent3D(faceExtent, faceExtent, 1), mipLevels, 6,
                              vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eSampled,
                              vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    vk::CommandBufferAllocateInfo cmdAi(*cmdPool_, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device_->device().allocateCommandBuffers(cmdAi);
    auto& cmd = cmds[0];
    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vk::ImageMemoryBarrier toTransfer(
        {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        *entry.image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 6));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {}, toTransfer);

    std::vector<vk::BufferImageCopy> regions;
    regions.reserve(static_cast<std::size_t>(mipLevels) * 6);
    vk::DeviceSize offset = 0;
    levelExtent = faceExtent;
    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        vk::DeviceSize faceSize =
            static_cast<vk::DeviceSize>(levelExtent) * levelExtent * 4 * sizeof(float);
        for (uint32_t face = 0; face < 6; ++face)
        {
            regions.emplace_back(
                offset, 0, 0,
                vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, level, face, 1),
                vk::Offset3D{0, 0, 0}, vk::Extent3D{levelExtent, levelExtent, 1});
            offset += faceSize;
        }
        levelExtent = std::max(1u, levelExtent / 2);
    }
    cmd.copyBufferToImage(*stagingBuf, *entry.image, vk::ImageLayout::eTransferDstOptimal, regions);

    vk::ImageMemoryBarrier toShader(
        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *entry.image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 6));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);
    cmd.end();

    vk::CommandBuffer rawCmd = *cmd;
    vk::SubmitInfo submitInfo({}, {}, rawCmd);
    device_->graphicsQueue().submit(submitInfo);
    device_->graphicsQueue().waitIdle();

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::eCube, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 6));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);
    createCubemapFaceViews(*device_, entry);

    auto props = device_->physicalDevice().getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, toVkFilter(sampler.magFilter), toVkFilter(sampler.minFilter),
        vk::SamplerMipmapMode::eLinear, toVkAddressMode(sampler.wrapS),
        toVkAddressMode(sampler.wrapT), toVkAddressMode(sampler.wrapS), 0.0f, vk::True,
        props.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways, 0.0f,
        static_cast<float>(mipLevels - 1), vk::BorderColor::eFloatOpaqueBlack, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

TextureHandle Resources::createRenderTargetCubemap(uint32_t faceExtent, uint32_t mipLevels,
                                                   vk::Format format,
                                                   const SamplerSettings& sampler)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = format;
    entry.mipLevels = mipLevels;

    // Transfer src/dst enable vkCmdBlitImage-based mip generation (used for
    // the skybox cubemap so the prefilter pass can do importance-sampled
    // mip-weighted lookups). Small memory/bandwidth cost when unused.
    vk::ImageCreateInfo imgCi(
        vk::ImageCreateFlagBits::eCubeCompatible, vk::ImageType::e2D, entry.format,
        vk::Extent3D(faceExtent, faceExtent, 1), mipLevels, 6, vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::eCube, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 6));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);
    createCubemapFaceViews(*device_, entry);

    auto props = device_->physicalDevice().getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, toVkFilter(sampler.magFilter), toVkFilter(sampler.minFilter),
        vk::SamplerMipmapMode::eLinear, toVkAddressMode(sampler.wrapS),
        toVkAddressMode(sampler.wrapT), toVkAddressMode(sampler.wrapS), 0.0f, vk::True,
        props.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways, 0.0f,
        static_cast<float>(mipLevels - 1), vk::BorderColor::eFloatOpaqueBlack, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

TextureHandle Resources::fallbackTexture(FallbackTextureKind kind)
{
    auto index = fallbackTextureIndex(kind);
    if (fallbackTextures_[index] == NullTexture)
    {
        fallbackTextures_[index] = createFallbackTexture(kind);
    }
    return fallbackTextures_[index];
}

TextureHandle Resources::createFallbackTexture(FallbackTextureKind kind)
{
    switch (kind)
    {
    case FallbackTextureKind::BaseColour:
    {
        static const uint8_t white[] = {255, 255, 255, 255};
        return createTexture(white, 1, 1, {}, TextureEncoding::Srgb);
    }
    case FallbackTextureKind::Emissive:
    {
        static const uint8_t black[] = {0, 0, 0, 255};
        return createTexture(black, 1, 1, {}, TextureEncoding::Srgb);
    }
    case FallbackTextureKind::Normal:
    {
        static const uint8_t flatNormal[] = {128, 128, 255, 255};
        return createTexture(flatNormal, 1, 1, {}, TextureEncoding::Linear);
    }
    case FallbackTextureKind::MetallicRoughness:
    case FallbackTextureKind::Occlusion:
    {
        static const uint8_t white[] = {255, 255, 255, 255};
        return createTexture(white, 1, 1, {}, TextureEncoding::Linear);
    }
    }

    return NullTexture;
}

// --- Shadow map ---

TextureHandle Resources::createShadowMap(uint32_t extent, uint32_t layerCount)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = vk::Format::eD32Sfloat;

    vk::ImageCreateInfo imgCi({}, vk::ImageType::e2D, entry.format, vk::Extent3D(extent, extent, 1),
                              1, layerCount, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                  vk::ImageUsageFlagBits::eSampled |
                                  vk::ImageUsageFlagBits::eTransferDst,
                              vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    // Main view: 2D for single-layer (sampler2DShadow), 2DArray for multi-layer
    // (sampler2DArrayShadow in the forward fragment shader).
    vk::ImageViewType mainViewType =
        layerCount > 1 ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, mainViewType, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, layerCount));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    // Per-layer 2D views for use as framebuffer attachments — one shadow
    // framebuffer binds one layer's depth via baseArrayLayer=L.
    if (layerCount > 1)
    {
        entry.faceViews.reserve(layerCount);
        for (uint32_t layer = 0; layer < layerCount; ++layer)
        {
            vk::ImageViewCreateInfo layerCi(
                {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, layer, 1));
            entry.faceViews.emplace_back(device_->device(), layerCi);
        }
    }

    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder, 0.0f, vk::False, 1.0f, vk::True,
        vk::CompareOp::eLess, 0.0f, 1.0f, vk::BorderColor::eFloatOpaqueWhite, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    // PCSS blocker search reads raw depth values, not comparison results, so
    // it needs a non-comparison sampler over the same image.
    vk::SamplerCreateInfo linearSamplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder, 0.0f, vk::False, 1.0f, vk::False,
        vk::CompareOp::eAlways, 0.0f, 1.0f, vk::BorderColor::eFloatOpaqueWhite, vk::False);
    entry.samplerLinear = vk::raii::Sampler(device_->device(), linearSamplerCi);

    return TextureHandle{id};
}

vk::ImageView Resources::vulkanShadowMapLayerView(TextureHandle handle,
                                                  uint32_t layer) const noexcept
{
    const auto& entry = textures_[static_cast<uint32_t>(handle)];
    return *entry.faceViews[layer];
}

vk::Sampler Resources::vulkanShadowSamplerLinear(TextureHandle handle) const noexcept
{
    return *textures_[static_cast<uint32_t>(handle)].samplerLinear;
}

TextureHandle Resources::createShadowColourAttachment(uint32_t extent)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = vk::Format::eB8G8R8A8Unorm;

    vk::ImageCreateInfo imgCi({}, vk::ImageType::e2D, entry.format, vk::Extent3D(extent, extent, 1),
                              1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eTransientAttachment,
                              vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    return TextureHandle{id};
}

TextureHandle Resources::createOffscreenColourTarget(vk::Extent2D extent)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = vk::Format::eR16G16B16A16Sfloat;

    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, entry.format, vk::Extent3D(extent.width, extent.height, 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge, 0.0f, vk::False, 1.0f, vk::False,
        vk::CompareOp::eAlways, 0.0f, 1.0f, vk::BorderColor::eFloatOpaqueBlack, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

TextureHandle Resources::createBloomChain(uint32_t width, uint32_t height, uint32_t mipLevels)
{
    auto id = static_cast<uint32_t>(textures_.size());
    textures_.emplace_back();
    auto& entry = textures_.back();
    entry.format = vk::Format::eR16G16B16A16Sfloat;
    entry.mipLevels = mipLevels;

    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, entry.format, vk::Extent3D(width, height, 1), mipLevels, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    entry.image = vk::raii::Image(device_->device(), imgCi);

    auto imgReq = entry.image.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device_->findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    entry.memory = vk::raii::DeviceMemory(device_->device(), imgAi);
    entry.image.bindMemory(*entry.memory, 0);

    // Main view spans all mips — used as the post-process bloom input via mip 0.
    vk::ImageViewCreateInfo viewCi(
        {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1));
    entry.view = vk::raii::ImageView(device_->device(), viewCi);

    // Per-mip 2D views — each downsample/upsample pass binds one as
    // framebuffer attachment (write) and another as shader input (read).
    entry.faceViews.reserve(mipLevels);
    for (uint32_t m = 0; m < mipLevels; ++m)
    {
        vk::ImageViewCreateInfo mipCi(
            {}, *entry.image, vk::ImageViewType::e2D, entry.format, {},
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, m, 1, 0, 1));
        entry.faceViews.emplace_back(device_->device(), mipCi);
    }

    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge, 0.0f, vk::False, 1.0f, vk::False,
        vk::CompareOp::eAlways, 0.0f, 0.0f, vk::BorderColor::eFloatOpaqueBlack, vk::False);
    entry.sampler = vk::raii::Sampler(device_->device(), samplerCi);

    return TextureHandle{id};
}

vk::ImageView Resources::vulkanBloomMipView(TextureHandle handle,
                                             uint32_t mipLevel) const noexcept
{
    return *textures_[static_cast<uint32_t>(handle)].faceViews[mipLevel];
}

void Resources::releaseTexture(TextureHandle handle)
{
    auto& entry = textures_[static_cast<uint32_t>(handle)];
    entry.sampler = vk::raii::Sampler{nullptr};
    entry.faceViews.clear();
    entry.view = vk::raii::ImageView{nullptr};
    entry.image = vk::raii::Image{nullptr};
    entry.memory = vk::raii::DeviceMemory{nullptr};
    entry.format = vk::Format::eUndefined;
    entry.mipLevels = 1;
}

Resources::DescriptorPoolEntry&
Resources::createDescriptorPool(std::span<const vk::DescriptorPoolSize> poolSizes, uint32_t maxSets)
{
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSets,
                                    poolSizes);
    auto& poolEntry = descriptorPools_.emplace_back();
    poolEntry.pool = vk::raii::DescriptorPool(device_->device(), ci);
    return poolEntry;
}

std::vector<vk::raii::DescriptorSet>
Resources::allocateDescriptorSets(vk::DescriptorPool pool, vk::DescriptorSetLayout layout,
                                  uint32_t count) const
{
    std::vector<vk::DescriptorSetLayout> layouts(count, layout);
    vk::DescriptorSetAllocateInfo ai(pool, layouts);
    return device_->device().allocateDescriptorSets(ai);
}

DescriptorSetHandle Resources::registerDescriptorSet(vk::DescriptorSet set)
{
    auto dsHandle = static_cast<uint32_t>(descriptorSetTable_.size());
    descriptorSetTable_.push_back(set);
    return DescriptorSetHandle{dsHandle};
}

void Resources::retainDescriptorSets(DescriptorPoolEntry& poolEntry,
                                     std::vector<vk::raii::DescriptorSet>& sets)
{
    for (auto& s : sets)
    {
        poolEntry.sets.push_back(std::move(s));
    }
}

// --- Mapped buffer sets ---

Resources::MappedBufferSet Resources::createMappedUniformBuffers(std::size_t size)
{
    MappedBufferSet result;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [buf, mem] = device_->createBuffer(
            static_cast<vk::DeviceSize>(size), vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        result.mapped[i] = mem.mapMemory(0, static_cast<vk::DeviceSize>(size));
        result.buffers[i] = storeBuffer(std::move(buf), std::move(mem));
    }
    return result;
}

Resources::MappedBufferSet Resources::createMappedStorageBuffer(std::size_t size,
                                                                const void* initialData)
{
    MappedBufferSet result;
    // Storage buffers are shared across frames — create a single buffer,
    // but fill both slots so callers can index by frame uniformly
    auto [buf, mem] = device_->createBuffer(
        static_cast<vk::DeviceSize>(size), vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
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

    std::array<vk::DescriptorPoolSize, 3> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, totalSets * 5},
        // 10 samplers per set: baseColor, emissive, normal, mr, occlusion,
        // shadowMap (compare), irradiance, prefiltered, brdfLut, shadowMapDepth.
        {vk::DescriptorType::eCombinedImageSampler, totalSets * 10},
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
            vk::DescriptorBufferInfo uboBufInfo(
                *buffers_[static_cast<uint32_t>(req.uniformBufs[i])].buffer, 0,
                sizeof(UniformBufferObject));
            vk::DescriptorBufferInfo matBufInfo(
                *buffers_[static_cast<uint32_t>(geo.materialBufs[i])].buffer, 0,
                sizeof(MaterialUBO));

            auto texIdx = static_cast<uint32_t>(geo.texture);
            vk::DescriptorImageInfo texInfo(*textures_[texIdx].sampler, *textures_[texIdx].view,
                                            vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::DescriptorBufferInfo skinBufInfo(
                *buffers_[static_cast<uint32_t>(geo.skinBufs[i])].buffer, 0, sizeof(SkinUBO));
            vk::DescriptorBufferInfo morphUboBufInfo(
                *buffers_[static_cast<uint32_t>(geo.morphUboBufs[i])].buffer, 0, sizeof(MorphUBO));

            vk::DeviceSize ssboSize = geo.morphSsboSize > 0
                                          ? static_cast<vk::DeviceSize>(geo.morphSsboSize)
                                          : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboBufInfo(
                *buffers_[static_cast<uint32_t>(geo.morphSsbo)].buffer, 0, ssboSize);

            auto emissiveTexIdx = static_cast<uint32_t>(geo.emissiveTexture);
            vk::DescriptorImageInfo emissiveTexInfo(*textures_[emissiveTexIdx].sampler,
                                                    *textures_[emissiveTexIdx].view,
                                                    vk::ImageLayout::eShaderReadOnlyOptimal);

            auto normalTexIdx = static_cast<uint32_t>(geo.normalTexture);
            vk::DescriptorImageInfo normalTexInfo(*textures_[normalTexIdx].sampler,
                                                  *textures_[normalTexIdx].view,
                                                  vk::ImageLayout::eShaderReadOnlyOptimal);

            auto mrTexIdx = static_cast<uint32_t>(geo.metallicRoughnessTexture);
            vk::DescriptorImageInfo mrTexInfo(*textures_[mrTexIdx].sampler,
                                              *textures_[mrTexIdx].view,
                                              vk::ImageLayout::eShaderReadOnlyOptimal);

            auto occTexIdx = static_cast<uint32_t>(geo.occlusionTexture);
            vk::DescriptorImageInfo occTexInfo(*textures_[occTexIdx].sampler,
                                               *textures_[occTexIdx].view,
                                               vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::DescriptorBufferInfo lightBufInfo(
                *buffers_[static_cast<uint32_t>(req.lightBufs[i])].buffer, 0, sizeof(LightUBO));

            auto shadowTexIdx = static_cast<uint32_t>(req.shadowMap);
            vk::DescriptorImageInfo shadowTexInfo(*textures_[shadowTexIdx].sampler,
                                                  *textures_[shadowTexIdx].view,
                                                  vk::ImageLayout::eDepthStencilReadOnlyOptimal);
            // Same shadow image, non-comparison sampler — read raw depths for
            // the PCSS blocker search.
            vk::DescriptorImageInfo shadowDepthTexInfo(
                *textures_[shadowTexIdx].samplerLinear, *textures_[shadowTexIdx].view,
                vk::ImageLayout::eDepthStencilReadOnlyOptimal);

            auto irradianceTexIdx = static_cast<uint32_t>(req.irradianceMap);
            vk::DescriptorImageInfo irradianceTexInfo(*textures_[irradianceTexIdx].sampler,
                                                      *textures_[irradianceTexIdx].view,
                                                      vk::ImageLayout::eShaderReadOnlyOptimal);

            auto prefilteredTexIdx = static_cast<uint32_t>(req.prefilteredMap);
            vk::DescriptorImageInfo prefilteredTexInfo(*textures_[prefilteredTexIdx].sampler,
                                                       *textures_[prefilteredTexIdx].view,
                                                       vk::ImageLayout::eShaderReadOnlyOptimal);

            auto brdfLutTexIdx = static_cast<uint32_t>(req.brdfLut);
            vk::DescriptorImageInfo brdfLutTexInfo(*textures_[brdfLutTexIdx].sampler,
                                                   *textures_[brdfLutTexIdx].view,
                                                   vk::ImageLayout::eShaderReadOnlyOptimal);

            std::array<vk::WriteDescriptorSet, 16> writes = {{
                {*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboBufInfo},
                {*sets[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &matBufInfo},
                {*sets[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
                {*sets[i], 3, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &skinBufInfo},
                {*sets[i], 4, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &morphUboBufInfo},
                {*sets[i], 5, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &morphSsboBufInfo},
                {*sets[i], 6, 0, 1, vk::DescriptorType::eCombinedImageSampler, &emissiveTexInfo},
                {*sets[i], 7, 0, 1, vk::DescriptorType::eCombinedImageSampler, &normalTexInfo},
                {*sets[i], 8, 0, 1, vk::DescriptorType::eCombinedImageSampler, &mrTexInfo},
                {*sets[i], 9, 0, 1, vk::DescriptorType::eCombinedImageSampler, &occTexInfo},
                {*sets[i], 10, 0, 1, vk::DescriptorType::eCombinedImageSampler, &shadowTexInfo},
                {*sets[i], 11, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &lightBufInfo},
                {*sets[i], 12, 0, 1, vk::DescriptorType::eCombinedImageSampler, &irradianceTexInfo},
                {*sets[i], 13, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                 &prefilteredTexInfo},
                {*sets[i], 14, 0, 1, vk::DescriptorType::eCombinedImageSampler, &brdfLutTexInfo},
                {*sets[i], 15, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                 &shadowDepthTexInfo},
            }};
            device_->device().updateDescriptorSets(writes, {});

            result.descSets[g][i] = registerDescriptorSet(*sets[i]);
        }

        retainDescriptorSets(poolEntry, sets);
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
            vk::DescriptorBufferInfo shadowUboInfo(
                *buffers_[static_cast<uint32_t>(geo.shadowUboBufs[i])].buffer, 0,
                sizeof(ShadowUBO));
            vk::DescriptorBufferInfo skinBufInfo(
                *buffers_[static_cast<uint32_t>(geo.skinBufs[i])].buffer, 0, sizeof(SkinUBO));
            vk::DescriptorBufferInfo morphUboBufInfo(
                *buffers_[static_cast<uint32_t>(geo.morphUboBufs[i])].buffer, 0, sizeof(MorphUBO));

            vk::DeviceSize ssboSize = geo.morphSsboSize > 0
                                          ? static_cast<vk::DeviceSize>(geo.morphSsboSize)
                                          : sizeof(float) * 4;
            vk::DescriptorBufferInfo morphSsboInfo(
                *buffers_[static_cast<uint32_t>(geo.morphSsbo)].buffer, 0, ssboSize);

            std::array<vk::WriteDescriptorSet, 4> writes = {{
                {*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &shadowUboInfo},
                {*sets[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &skinBufInfo},
                {*sets[i], 2, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &morphUboBufInfo},
                {*sets[i], 3, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &morphSsboInfo},
            }};
            device_->device().updateDescriptorSets(writes, {});

            result.descSets[g][i] = registerDescriptorSet(*sets[i]);
        }

        retainDescriptorSets(poolEntry, sets);
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
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo bufInfo(*buffers_[static_cast<uint32_t>(ubo.buffers[i])].buffer, 0,
                                         uboSize);
        vk::WriteDescriptorSet write(*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr,
                                     &bufInfo);
        device_->device().updateDescriptorSets(write, {});

        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);

    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Resources::createUboImageSamplerDescriptors(vk::DescriptorSetLayout layout,
                                            const MappedBufferSet& ubo, vk::DeviceSize uboSize,
                                            TextureHandle texture)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
        {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    auto texIdx = static_cast<uint32_t>(texture);
    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo bufInfo(*buffers_[static_cast<uint32_t>(ubo.buffers[i])].buffer, 0,
                                         uboSize);
        vk::DescriptorImageInfo texInfo(*textures_[texIdx].sampler, *textures_[texIdx].view,
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 2> writes = {{
            {*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufInfo},
            {*sets[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
        }};
        device_->device().updateDescriptorSets(writes, {});
        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);
    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Resources::createSkyboxDescriptors(vk::DescriptorSetLayout layout, const MappedBufferSet& skyboxUbo,
                                   vk::DeviceSize skyboxUboSize, TextureHandle texture,
                                   const MappedBufferSet& lightUbo, vk::DeviceSize lightUboSize)
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT * 2},
        {vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, MAX_FRAMES_IN_FLIGHT);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, MAX_FRAMES_IN_FLIGHT);

    auto texIdx = static_cast<uint32_t>(texture);
    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> result{};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo skyboxBufInfo(
            *buffers_[static_cast<uint32_t>(skyboxUbo.buffers[i])].buffer, 0, skyboxUboSize);
        vk::DescriptorBufferInfo lightBufInfo(
            *buffers_[static_cast<uint32_t>(lightUbo.buffers[i])].buffer, 0, lightUboSize);
        vk::DescriptorImageInfo texInfo(*textures_[texIdx].sampler, *textures_[texIdx].view,
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 3> writes = {{
            {*sets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &skyboxBufInfo},
            {*sets[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
            {*sets[i], 2, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &lightBufInfo},
        }};
        device_->device().updateDescriptorSets(writes, {});
        result[i] = registerDescriptorSet(*sets[i]);
    }

    retainDescriptorSets(poolEntry, sets);
    return result;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Resources::createSingleImageSamplerDescriptors(vk::DescriptorSetLayout layout,
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

void Resources::updateSingleImageSamplerDescriptors(
    const std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>& sets, TextureHandle texture)
{
    auto& entry = textures_[static_cast<uint32_t>(texture)];
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorImageInfo imgInfo(*entry.sampler, *entry.view,
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
        vk::WriteDescriptorSet write(descriptorSetTable_[static_cast<uint32_t>(sets[i])], 0, 0, 1,
                                     vk::DescriptorType::eCombinedImageSampler, &imgInfo);
        device_->device().updateDescriptorSets(write, {});
    }
}

DescriptorSetHandle Resources::createImageViewDescriptor(vk::DescriptorSetLayout layout,
                                                         vk::ImageView view, vk::Sampler sampler)
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes = {{
        {vk::DescriptorType::eCombinedImageSampler, 1},
    }};
    auto& poolEntry = createDescriptorPool(poolSizes, 1);
    auto sets = allocateDescriptorSets(*poolEntry.pool, layout, 1);

    vk::DescriptorImageInfo imgInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet write(*sets[0], 0, 0, 1, vk::DescriptorType::eCombinedImageSampler,
                                 &imgInfo);
    device_->device().updateDescriptorSets(write, {});

    auto handle = registerDescriptorSet(*sets[0]);
    retainDescriptorSets(poolEntry, sets);
    return handle;
}

std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
Resources::createPostProcessDescriptors(vk::DescriptorSetLayout layout, TextureHandle hdrTarget,
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

void Resources::updatePostProcessDescriptors(
    const std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>& sets, TextureHandle hdrTarget,
    TextureHandle bloomChain)
{
    auto& hdrEntry = textures_[static_cast<uint32_t>(hdrTarget)];
    auto& bloomEntry = textures_[static_cast<uint32_t>(bloomChain)];
    // Bloom binding samples mip 0 of the chain (the final upsampled result).
    vk::ImageView bloomMip0 = *bloomEntry.faceViews[0];

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorImageInfo hdrInfo(*hdrEntry.sampler, *hdrEntry.view,
                                        vk::ImageLayout::eShaderReadOnlyOptimal);
        vk::DescriptorImageInfo bloomInfo(*bloomEntry.sampler, bloomMip0,
                                          vk::ImageLayout::eShaderReadOnlyOptimal);
        std::array<vk::WriteDescriptorSet, 2> writes = {{
            {descriptorSetTable_[static_cast<uint32_t>(sets[i])], 0, 0, 1,
             vk::DescriptorType::eCombinedImageSampler, &hdrInfo},
            {descriptorSetTable_[static_cast<uint32_t>(sets[i])], 1, 0, 1,
             vk::DescriptorType::eCombinedImageSampler, &bloomInfo},
        }};
        device_->device().updateDescriptorSets(writes, {});
    }
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

vk::ImageView Resources::vulkanCubemapFaceView(TextureHandle handle, uint32_t face,
                                               uint32_t mipLevel) const noexcept
{
    const auto& entry = textures_[static_cast<uint32_t>(handle)];
    std::size_t index = static_cast<std::size_t>(mipLevel) * 6 + face;
    return *entry.faceViews[index];
}

vk::Image Resources::vulkanImage(TextureHandle handle) const noexcept
{
    return *textures_[static_cast<uint32_t>(handle)].image;
}

vk::Sampler Resources::vulkanSampler(TextureHandle handle) const noexcept
{
    return *textures_[static_cast<uint32_t>(handle)].sampler;
}

vk::Format Resources::textureFormat(TextureHandle handle) const noexcept
{
    return textures_[static_cast<uint32_t>(handle)].format;
}

uint32_t Resources::textureMipLevels(TextureHandle handle) const noexcept
{
    return textures_[static_cast<uint32_t>(handle)].mipLevels;
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
