#include <fire_engine/graphics/texture.hpp>

#include <cstring>

#include <fire_engine/render/device.hpp>

namespace fire_engine
{

Texture Texture::load_from_file(const std::string& path, const Device& device,
                                vk::CommandPool cmdPool)
{
    Image img = Image::load_from_file(path);
    return load_from_data(img.data(), img.width(), img.height(), device, cmdPool);
}

Texture Texture::load_from_data(const uint8_t* pixels, int width, int height, const Device& device,
                                vk::CommandPool cmdPool)
{
    Texture tex;
    vk::DeviceSize imageSize =
        static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4;

    vk::raii::DeviceMemory stagingMem{nullptr};
    auto stagingBuf = createStagingBuffer(device, pixels, imageSize, stagingMem);

    createTextureImage(tex, device, width, height);
    transitionAndCopyImage(device, cmdPool, stagingBuf, tex, width, height);
    createImageViewAndSampler(tex, device);

    return tex;
}

vk::raii::Buffer Texture::createStagingBuffer(const Device& device, const uint8_t* pixels,
                                              vk::DeviceSize imageSize,
                                              vk::raii::DeviceMemory& stagingMem)
{
    auto [buf, mem] = device.createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                                          vk::MemoryPropertyFlagBits::eHostVisible |
                                              vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = mem.mapMemory(0, imageSize);
    std::memcpy(data, pixels, imageSize);
    mem.unmapMemory();

    stagingMem = std::move(mem);
    return std::move(buf);
}

void Texture::createTextureImage(Texture& tex, const Device& device, int width, int height)
{
    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    tex.image_ = vk::raii::Image(device.device(), imgCi);

    auto imgReq = tex.image_.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(
        imgReq.size,
        device.findMemoryType(imgReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    tex.memory_ = vk::raii::DeviceMemory(device.device(), imgAi);
    tex.image_.bindMemory(*tex.memory_, 0);
}

void Texture::transitionAndCopyImage(const Device& device, vk::CommandPool cmdPool,
                                     const vk::raii::Buffer& stagingBuf, Texture& tex, int width,
                                     int height)
{
    vk::CommandBufferAllocateInfo cmdAi(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device.device().allocateCommandBuffers(cmdAi);
    auto& cmd = cmds[0];

    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Transition: undefined -> transfer dst
    vk::ImageMemoryBarrier toTransfer(
        {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        *tex.image_, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {}, toTransfer);

    // Copy buffer to image
    vk::BufferImageCopy region(
        0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), {0, 0, 0},
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1});
    cmd.copyBufferToImage(*stagingBuf, *tex.image_, vk::ImageLayout::eTransferDstOptimal, region);

    // Transition: transfer dst -> shader read
    vk::ImageMemoryBarrier toShader(
        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, *tex.image_,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);

    cmd.end();

    vk::CommandBuffer rawCmd = *cmd;
    vk::SubmitInfo submitInfo({}, {}, rawCmd);
    device.graphicsQueue().submit(submitInfo);
    device.graphicsQueue().waitIdle();
}

void Texture::createImageViewAndSampler(Texture& tex, const Device& device)
{
    vk::ImageViewCreateInfo viewCi(
        {}, *tex.image_, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    tex.view_ = vk::raii::ImageView(device.device(), viewCi);

    auto props = device.physicalDevice().getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat, 0.0f, vk::True, props.limits.maxSamplerAnisotropy,
        vk::False, vk::CompareOp::eAlways, 0.0f, 0.0f, vk::BorderColor::eIntOpaqueBlack, vk::False);
    tex.sampler_ = vk::raii::Sampler(device.device(), samplerCi);
}

} // namespace fire_engine
