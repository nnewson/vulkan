#include <fire_engine/graphics/texture.hpp>

#include <cstring>
#include <stdexcept>

namespace fire_engine
{

Texture Texture::load_from_file(const std::string& path, const vk::raii::Device& device,
                                const vk::raii::PhysicalDevice& physDevice,
                                vk::CommandPool cmdPool, const vk::raii::Queue& queue)
{
    Image img = Image::load_from_file(path);
    return load_from_data(img.data(), img.width(), img.height(), device, physDevice, cmdPool, queue);
}

Texture Texture::load_from_data(const uint8_t* pixels, int width, int height,
                                const vk::raii::Device& device,
                                const vk::raii::PhysicalDevice& physDevice,
                                vk::CommandPool cmdPool, const vk::raii::Queue& queue)
{
    Texture tex;
    vk::DeviceSize imageSize =
        static_cast<vk::DeviceSize>(width) * static_cast<vk::DeviceSize>(height) * 4;

    // Create staging buffer (RAII — auto-destroyed at end of scope)
    vk::BufferCreateInfo bufCi({}, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                               vk::SharingMode::eExclusive);
    vk::raii::Buffer stagingBuf(device, bufCi);

    auto bufReq = stagingBuf.getMemoryRequirements();
    vk::MemoryAllocateInfo bufAi(bufReq.size,
                                 findMemoryType(physDevice, bufReq.memoryTypeBits,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent));
    vk::raii::DeviceMemory stagingMem(device, bufAi);
    stagingBuf.bindMemory(*stagingMem, 0);

    void* data = stagingMem.mapMemory(0, imageSize);
    std::memcpy(data, pixels, imageSize);
    stagingMem.unmapMemory();

    // Create Vulkan image
    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    tex.image_ = vk::raii::Image(device, imgCi);

    auto imgReq = tex.image_.getMemoryRequirements();
    vk::MemoryAllocateInfo imgAi(imgReq.size,
                                 findMemoryType(physDevice, imgReq.memoryTypeBits,
                                                vk::MemoryPropertyFlagBits::eDeviceLocal));
    tex.memory_ = vk::raii::DeviceMemory(device, imgAi);
    tex.image_.bindMemory(*tex.memory_, 0);

    // Record commands: transition + copy + transition
    vk::CommandBufferAllocateInfo cmdAi(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device.allocateCommandBuffers(cmdAi);
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
    queue.submit(submitInfo);
    queue.waitIdle();
    // cmd and staging buffer/mem auto-destroyed at end of scope

    // Create image view
    vk::ImageViewCreateInfo viewCi(
        {}, *tex.image_, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    tex.view_ = vk::raii::ImageView(device, viewCi);

    // Create sampler
    auto props = physDevice.getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat, 0.0f, vk::True, props.limits.maxSamplerAnisotropy,
        vk::False, vk::CompareOp::eAlways, 0.0f, 0.0f, vk::BorderColor::eIntOpaqueBlack, vk::False);
    tex.sampler_ = vk::raii::Sampler(device, samplerCi);

    return tex;
}

uint32_t Texture::findMemoryType(const vk::raii::PhysicalDevice& physDevice, uint32_t filter,
                                 vk::MemoryPropertyFlags props)
{
    auto mem = physDevice.getMemoryProperties();
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
    {
        if ((filter & (1 << i)) && (mem.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type for texture");
}

} // namespace fire_engine
