#pragma once

#include <fire_engine/renderer/device.hpp>
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

    void waitIdle() const
    {
        device_.device().waitIdle();
    }

private:
    Device device_;
    Swapchain swapchain_;
    Pipeline pipeline_;
};

} // namespace fire_engine
