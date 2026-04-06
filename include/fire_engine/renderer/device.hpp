#pragma once

#include <optional>
#include <utility>

#include <vulkan/vulkan.hpp>

#include <fire_engine/platform/window.hpp>

namespace fire_engine
{

class Device
{
public:
    explicit Device(const Window& window);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) noexcept = default;
    Device& operator=(Device&&) noexcept = default;

    [[nodiscard]] vk::Instance instance() const noexcept { return instance_; }
    [[nodiscard]] vk::SurfaceKHR surface() const noexcept { return surface_; }
    [[nodiscard]] vk::PhysicalDevice physicalDevice() const noexcept { return physDevice_; }
    [[nodiscard]] vk::Device device() const noexcept { return device_; }
    [[nodiscard]] vk::Queue graphicsQueue() const noexcept { return graphicsQueue_; }
    [[nodiscard]] vk::Queue presentQueue() const noexcept { return presentQueue_; }
    [[nodiscard]] uint32_t graphicsFamily() const noexcept { return graphicsFamily_; }
    [[nodiscard]] uint32_t presentFamily() const noexcept { return presentFamily_; }

    [[nodiscard]] uint32_t findMemoryType(uint32_t filter, vk::MemoryPropertyFlags props) const;

private:
    void createInstance();
    void createSurface(const Window& window);
    void pickPhysicalDevice();
    void createLogicalDevice();

    [[nodiscard]] bool isDeviceSuitable(vk::PhysicalDevice d);
    [[nodiscard]] std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
    findQueueFamilies(vk::PhysicalDevice d);

    vk::Instance instance_;
    vk::SurfaceKHR surface_;
    vk::PhysicalDevice physDevice_;
    vk::Device device_;
    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
    uint32_t graphicsFamily_{0};
    uint32_t presentFamily_{0};
};

} // namespace fire_engine
