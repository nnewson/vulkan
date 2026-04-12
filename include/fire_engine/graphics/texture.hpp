#pragma once

#include <string>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/graphics/image.hpp>

namespace fire_engine
{

class Device;

class Texture
{
public:
    static Texture load_from_file(const std::string& path, const Device& device,
                                  vk::CommandPool cmdPool);

    static Texture load_from_data(const uint8_t* pixels, int width, int height,
                                  const Device& device, vk::CommandPool cmdPool);

    Texture() = default;
    ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;

    [[nodiscard]]
    vk::ImageView view() const noexcept
    {
        return *view_;
    }

    [[nodiscard]]
    vk::Sampler sampler() const noexcept
    {
        return *sampler_;
    }

private:
    static vk::raii::Buffer createStagingBuffer(const Device& device, const uint8_t* pixels,
                                                vk::DeviceSize imageSize,
                                                vk::raii::DeviceMemory& stagingMem);

    static void createTextureImage(Texture& tex, const Device& device, int width, int height);

    static void transitionAndCopyImage(const Device& device, vk::CommandPool cmdPool,
                                       const vk::raii::Buffer& stagingBuf, Texture& tex, int width,
                                       int height);

    static void createImageViewAndSampler(Texture& tex, const Device& device);

    vk::raii::Image image_{nullptr};
    vk::raii::DeviceMemory memory_{nullptr};
    vk::raii::ImageView view_{nullptr};
    vk::raii::Sampler sampler_{nullptr};
};

} // namespace fire_engine
