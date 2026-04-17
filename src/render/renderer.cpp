#include <fire_engine/render/renderer.hpp>

#include <limits>
#include <tuple>

#include <fire_engine/render/render_context.hpp>
#include <fire_engine/render/swapchain.hpp>
#include <fire_engine/render/ubo.hpp>
#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

Renderer::Renderer(const Window& window)
    : device_(window),
      swapchain_(device_, window),
      forwardRenderPass_(Pipeline::createForwardRenderPass(device_, swapchain_)),
      pipeline_(device_, swapchain_, Pipeline::forwardConfig(*forwardRenderPass_)),
      frame_(device_, swapchain_),
      resources_(device_, pipeline_)
{
    swapchain_.createDepthResources(device_);
    swapchain_.createFramebuffers(*forwardRenderPass_);
    forwardPipeline_ = resources_.registerPipeline(pipeline_.pipeline(), pipeline_.pipelineLayout());
}

void Renderer::drawFrame(Window& display, SceneGraph& scene, Vec3 cameraPosition, Vec3 cameraTarget)
{
    auto imageIndex = acquireNextImage(display);
    if (!imageIndex)
    {
        return;
    }

    auto cmd = frame_.commandBuffer(currentFrame_);
    cmd.reset();
    cmd.begin(vk::CommandBufferBeginInfo{});

    beginRenderPass(cmd, *imageIndex);

    std::vector<DrawCommand> drawCommands;
    RenderContext ctx{device_,       swapchain_,     frame_,        pipeline_,
                      cmd,           currentFrame_,  cameraPosition, cameraTarget,
                      &drawCommands, forwardPipeline_};
    scene.render(ctx);

    // Record draw commands collected during scene traversal. Pipeline is
    // bound via the DrawCommand-carried PipelineHandle so that future passes
    // with different pipelines can interleave without changing this loop.
    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    for (const auto& dc : drawCommands)
    {
        if (dc.pipeline != lastBoundPipeline)
        {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             resources_.vulkanPipeline(dc.pipeline));
            lastBoundPipeline = dc.pipeline;
        }
        cmd.bindVertexBuffers(0, resources_.vulkanBuffer(dc.vertexBuffer), {vk::DeviceSize{0}});
        cmd.bindIndexBuffer(resources_.vulkanBuffer(dc.indexBuffer), 0, vk::IndexType::eUint16);
        vk::DescriptorSet ds = resources_.vulkanDescriptorSet(dc.descriptorSet);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               resources_.vulkanPipelineLayout(dc.pipeline), 0, ds, {});
        cmd.drawIndexed(dc.indexCount, 1, 0, 0, 0);
    }

    cmd.endRenderPass();
    cmd.end();

    submitAndPresent(display, cmd, *imageIndex);
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

std::optional<uint32_t> Renderer::acquireNextImage(Window& display)
{
    auto& dev = device_.device();

    (void)dev.waitForFences(frame_.inFlightFence(currentFrame_), vk::True, UINT64_MAX);

    auto [acquireResult, imageIndex] = (*dev).acquireNextImageKHR(
        swapchain_.swapchain(), UINT64_MAX, frame_.imageAvailable(currentFrame_));
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain(display);
        return std::nullopt;
    }
    if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    dev.resetFences(frame_.inFlightFence(currentFrame_));
    return imageIndex;
}

void Renderer::beginRenderPass(vk::CommandBuffer cmd, uint32_t imageIndex)
{
    auto extent = swapchain_.extent();
    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue(std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f});
    clears[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo rpBegin(*forwardRenderPass_, swapchain_.framebuffer(imageIndex),
                                    vk::Rect2D({0, 0}, extent), clears);

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);
}

void Renderer::submitAndPresent(Window& display, vk::CommandBuffer cmd, uint32_t imageIndex)
{
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto imageAvail = frame_.imageAvailable(currentFrame_);
    auto renderDone = frame_.renderFinished(imageIndex);
    vk::SubmitInfo si(imageAvail, waitStage, cmd, renderDone);

    device_.graphicsQueue().submit(si, frame_.inFlightFence(currentFrame_));

    auto swapchain = swapchain_.swapchain();
    vk::PresentInfoKHR pi(renderDone, swapchain, imageIndex);

    vk::Result presentResult = device_.presentQueue().presentKHR(pi);
    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR || display.framebufferResized())
    {
        display.framebufferResized(false);
        recreateSwapchain(display);
    }
    else if (presentResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image");
    }
}

void Renderer::recreateSwapchain(const Window& display)
{
    auto [w, h] = display.framebufferSize();
    while (w == 0 || h == 0)
    {
        std::tie(w, h) = display.framebufferSize();
        Window::waitEvents();
    }
    waitIdle();

    frame_.destroyRenderFinishedSemaphores();

    swapchain_.recreate(device_, display, *forwardRenderPass_);

    frame_.createRenderFinishedSemaphores(swapchain_.images().size());
}

} // namespace fire_engine
