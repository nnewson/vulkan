#pragma once

#include <fire_engine/math/vec3.hpp>
#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/frame.hpp>
#include <fire_engine/renderer/pipeline.hpp>
#include <fire_engine/renderer/swapchain.hpp>

namespace fire_engine
{

class Renderer
{
public:
    explicit Renderer(const Window& window);
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    void drawFrame(Window& display, Vec3 cameraPos, Vec3 cameraTarget);

    void waitIdle() const
    {
        device_.device().waitIdle();
    }

    [[nodiscard]] const Device& device() const noexcept
    {
        return device_;
    }

    [[nodiscard]] const Swapchain& swapchain() const noexcept
    {
        return swapchain_;
    }

    [[nodiscard]] Swapchain& swapchain() noexcept
    {
        return swapchain_;
    }

    [[nodiscard]] const Pipeline& pipeline() const noexcept
    {
        return pipeline_;
    }

    [[nodiscard]] const Frame& frame() const noexcept
    {
        return frame_;
    }

    [[nodiscard]] Frame& frame() noexcept
    {
        return frame_;
    }

private:
    void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex);
    void updateUniformBuffer(Vec3 cameraPos, Vec3 cameraTarget);
    void recreateSwapchain(const Window& display);

    Device device_;
    Swapchain swapchain_;
    Pipeline pipeline_;
    Frame frame_;
    uint32_t currentFrame_{0};
};

} // namespace fire_engine
