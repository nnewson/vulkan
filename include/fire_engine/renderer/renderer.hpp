#pragma once

#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/frame.hpp>
#include <fire_engine/renderer/pipeline.hpp>
#include <fire_engine/renderer/swapchain.hpp>

namespace fire_engine
{

class SceneGraph;

class Renderer
{
public:
    explicit Renderer(const Window& window);
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    void drawFrame(Window& display, SceneGraph& scene);

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
    void recreateSwapchain(const Window& display);

    Device device_;
    Swapchain swapchain_;
    Pipeline pipeline_;
    Frame frame_;
    uint32_t currentFrame_{0};
};

} // namespace fire_engine
