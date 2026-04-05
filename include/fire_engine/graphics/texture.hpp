#pragma once

#include <string>

#include <vulkan/vulkan.hpp>

#include <fire_engine/graphics/image.hpp>

namespace fire_engine
{

class Texture
{
public:
    static Texture load_from_file(const std::string& path, vk::Device device,
                                  vk::PhysicalDevice physDevice, vk::CommandPool cmdPool,
                                  vk::Queue queue);

    Texture() = default;
    ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void destroy(vk::Device device);

    [[nodiscard]]
    vk::ImageView view() const noexcept
    {
        return view_;
    }

    [[nodiscard]]
    vk::Sampler sampler() const noexcept
    {
        return sampler_;
    }

private:
    static uint32_t findMemoryType(vk::PhysicalDevice physDevice, uint32_t filter,
                                   vk::MemoryPropertyFlags props);

    vk::Image image_;
    vk::DeviceMemory memory_;
    vk::ImageView view_;
    vk::Sampler sampler_;
};

} // namespace fire_engine
