#pragma once

#include <optional>

#include <fire_engine/math/vec3.hpp>
#include <fire_engine/render/device.hpp>
#include <fire_engine/render/frame.hpp>
#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/resources.hpp>
#include <fire_engine/render/swapchain.hpp>

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

    void drawFrame(Window& display, SceneGraph& scene, Vec3 cameraPosition, Vec3 cameraTarget);

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

    [[nodiscard]] Resources& resources() noexcept
    {
        return resources_;
    }

    [[nodiscard]] const Resources& resources() const noexcept
    {
        return resources_;
    }

private:
    void recreateSwapchain(const Window& display);
    [[nodiscard]] std::optional<uint32_t> acquireNextImage(Window& display);
    void beginRenderPass(vk::CommandBuffer cmd, uint32_t imageIndex);
    void submitAndPresent(Window& display, vk::CommandBuffer cmd, uint32_t imageIndex);

    Device device_;
    Swapchain swapchain_;
    Pipeline pipeline_;
    Frame frame_;
    Resources resources_;
    PipelineHandle forwardPipeline_{NullPipeline};
    uint32_t currentFrame_{0};
};

} // namespace fire_engine
