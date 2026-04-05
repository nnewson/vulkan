#include <fire_engine/graphics/texture.hpp>

#include <cstring>
#include <stdexcept>

namespace fire_engine
{

Texture Texture::load_from_file(const std::string& path, vk::Device device,
                                vk::PhysicalDevice physDevice, vk::CommandPool cmdPool,
                                vk::Queue queue)
{
    Image img = Image::load_from_file(path);

    Texture tex;
    vk::DeviceSize imageSize = img.size_bytes();

    // Create staging buffer
    vk::Buffer stagingBuf;
    vk::DeviceMemory stagingMem;

    vk::BufferCreateInfo bufCi({}, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                               vk::SharingMode::eExclusive);
    stagingBuf = device.createBuffer(bufCi);

    auto bufReq = device.getBufferMemoryRequirements(stagingBuf);
    vk::MemoryAllocateInfo bufAi(bufReq.size,
                                 findMemoryType(physDevice, bufReq.memoryTypeBits,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent));
    stagingMem = device.allocateMemory(bufAi);
    device.bindBufferMemory(stagingBuf, stagingMem, 0);

    void* data = device.mapMemory(stagingMem, 0, imageSize);
    std::memcpy(data, img.data(), imageSize);
    device.unmapMemory(stagingMem);

    // Create Vulkan image
    vk::ImageCreateInfo imgCi(
        {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
        vk::Extent3D(static_cast<uint32_t>(img.width()), static_cast<uint32_t>(img.height()), 1),
        1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    tex.image_ = device.createImage(imgCi);

    auto imgReq = device.getImageMemoryRequirements(tex.image_);
    vk::MemoryAllocateInfo imgAi(imgReq.size,
                                 findMemoryType(physDevice, imgReq.memoryTypeBits,
                                                vk::MemoryPropertyFlagBits::eDeviceLocal));
    tex.memory_ = device.allocateMemory(imgAi);
    device.bindImageMemory(tex.image_, tex.memory_, 0);

    // Record commands: transition + copy + transition
    vk::CommandBufferAllocateInfo cmdAi(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
    auto cmds = device.allocateCommandBuffers(cmdAi);
    vk::CommandBuffer cmd = cmds[0];

    cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Transition: undefined -> transfer dst
    vk::ImageMemoryBarrier toTransfer(
        {}, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        tex.image_, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, toTransfer);

    // Copy buffer to image
    vk::BufferImageCopy region(
        0, 0, 0,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), {0, 0, 0},
        {static_cast<uint32_t>(img.width()), static_cast<uint32_t>(img.height()), 1});
    cmd.copyBufferToImage(stagingBuf, tex.image_, vk::ImageLayout::eTransferDstOptimal, region);

    // Transition: transfer dst -> shader read
    vk::ImageMemoryBarrier toShader(
        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, tex.image_,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, toShader);

    cmd.end();

    vk::SubmitInfo submitInfo({}, {}, cmd);
    queue.submit(submitInfo);
    queue.waitIdle();

    device.freeCommandBuffers(cmdPool, cmd);
    device.destroyBuffer(stagingBuf);
    device.freeMemory(stagingMem);

    // Create image view
    vk::ImageViewCreateInfo viewCi({}, tex.image_, vk::ImageViewType::e2D,
                                   vk::Format::eR8G8B8A8Srgb, {},
                                   vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1,
                                                             0, 1));
    tex.view_ = device.createImageView(viewCi);

    // Create sampler
    auto props = physDevice.getProperties();
    vk::SamplerCreateInfo samplerCi(
        {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat, 0.0f, vk::True,
        props.limits.maxSamplerAnisotropy, vk::False, vk::CompareOp::eAlways, 0.0f, 0.0f,
        vk::BorderColor::eIntOpaqueBlack, vk::False);
    tex.sampler_ = device.createSampler(samplerCi);

    return tex;
}

void Texture::destroy(vk::Device device)
{
    if (sampler_)
        device.destroySampler(sampler_);
    if (view_)
        device.destroyImageView(view_);
    if (image_)
        device.destroyImage(image_);
    if (memory_)
        device.freeMemory(memory_);
}

Texture::Texture(Texture&& other) noexcept
    : image_(other.image_),
      memory_(other.memory_),
      view_(other.view_),
      sampler_(other.sampler_)
{
    other.image_ = nullptr;
    other.memory_ = nullptr;
    other.view_ = nullptr;
    other.sampler_ = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        image_ = other.image_;
        memory_ = other.memory_;
        view_ = other.view_;
        sampler_ = other.sampler_;
        other.image_ = nullptr;
        other.memory_ = nullptr;
        other.view_ = nullptr;
        other.sampler_ = nullptr;
    }
    return *this;
}

uint32_t Texture::findMemoryType(vk::PhysicalDevice physDevice, uint32_t filter,
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
