#include <fire_engine/render/renderer.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <tuple>

#include <fire_engine/math/constants.hpp>
#include <fire_engine/render/render_context.hpp>
#include <fire_engine/render/swapchain.hpp>
#include <fire_engine/render/ubo.hpp>
#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

Renderer::Renderer(const Window& window)
    : device_(window),
      swapchain_(device_, window),
      forwardPass_(RenderPass::createForward(device_, swapchain_)),
      pipelineOpaque_(device_, Pipeline::forwardConfig(forwardPass_.renderPass())),
      pipelineOpaqueDoubleSided_(device_,
                                 Pipeline::forwardDoubleSidedConfig(forwardPass_.renderPass())),
      pipelineBlend_(device_,
                     Pipeline::forwardBlendConfig(forwardPass_.renderPass())),
      skyboxPipeline_(device_, Pipeline::skyboxConfig(forwardPass_.renderPass())),
      frame_(device_, swapchain_),
      resources_(device_, pipelineOpaque_)
{
    swapchain_.createDepthResources(device_);
    forwardPass_.createForwardFramebuffers(device_, swapchain_);
    forwardOpaqueHandle_ =
        resources_.registerPipeline(pipelineOpaque_.pipeline(), pipelineOpaque_.pipelineLayout());
    forwardOpaqueDoubleSidedHandle_ = resources_.registerPipeline(
        pipelineOpaqueDoubleSided_.pipeline(), pipelineOpaqueDoubleSided_.pipelineLayout());
    forwardBlendHandle_ =
        resources_.registerPipeline(pipelineBlend_.pipeline(), pipelineBlend_.pipelineLayout());
    skyboxPipelineHandle_ = resources_.registerPipeline(skyboxPipeline_.pipeline(),
                                                        skyboxPipeline_.pipelineLayout());
    skyboxUbo_ = resources_.createMappedUniformBuffers(sizeof(SkyboxUBO));
    skyboxDescSets_ = resources_.createSingleUboDescriptors(
        skyboxPipeline_.descriptorSetLayout(), skyboxUbo_, sizeof(SkyboxUBO));
    std::array<uint16_t, 3> skyboxIndices{0, 1, 2};
    skyboxIndexBuffer_ = resources_.createIndexBuffer(skyboxIndices);
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
    recordSkybox(cameraPosition, cameraTarget, drawCommands);

    AlphaPipelines pipelines{forwardOpaqueHandle_, forwardOpaqueDoubleSidedHandle_,
                             forwardBlendHandle_};
    RenderContext ctx{device_,       swapchain_,    frame_,         pipelineOpaque_,
                      cmd,           currentFrame_, cameraPosition, cameraTarget,
                      &drawCommands, pipelines};
    scene.render(ctx);

    // Split draws into opaque (includes skybox + MASK) and blend buckets.
    // Blend is sorted back-to-front by world-space centroid depth so straight
    // alpha composites in the right order. Opaque keeps emission order.
    std::vector<DrawCommand> opaqueDraws;
    std::vector<DrawCommand> blendDraws;
    opaqueDraws.reserve(drawCommands.size());
    for (const auto& dc : drawCommands)
    {
        if (dc.pipeline == forwardBlendHandle_)
        {
            blendDraws.push_back(dc);
        }
        else
        {
            opaqueDraws.push_back(dc);
        }
    }
    std::sort(blendDraws.begin(), blendDraws.end(),
              [](const DrawCommand& a, const DrawCommand& b) {
                  return a.sortDepth > b.sortDepth;
              });

    // Record draw commands. Pipeline is bound via the DrawCommand-carried
    // PipelineHandle so passes with different pipelines interleave without
    // changing this loop.
    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    auto recordBucket = [&](const std::vector<DrawCommand>& bucket) {
        for (const auto& dc : bucket)
        {
            if (dc.pipeline != lastBoundPipeline)
            {
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                 resources_.vulkanPipeline(dc.pipeline));
                lastBoundPipeline = dc.pipeline;
            }
            if (dc.vertexBuffer != NullBuffer)
            {
                cmd.bindVertexBuffers(0, resources_.vulkanBuffer(dc.vertexBuffer),
                                      {vk::DeviceSize{0}});
            }
            cmd.bindIndexBuffer(resources_.vulkanBuffer(dc.indexBuffer), 0,
                                vk::IndexType::eUint16);
            vk::DescriptorSet ds = resources_.vulkanDescriptorSet(dc.descriptorSet);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   resources_.vulkanPipelineLayout(dc.pipeline), 0, ds, {});
            cmd.drawIndexed(dc.indexCount, 1, 0, 0, 0);
        }
    };
    recordBucket(opaqueDraws);
    recordBucket(blendDraws);

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

    vk::RenderPassBeginInfo rpBegin(forwardPass_.renderPass(),
                                    forwardPass_.framebuffer(imageIndex),
                                    vk::Rect2D({0, 0}, extent), clears);

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

    vk::Viewport viewport(0, 0, static_cast<float>(extent.width),
                          static_cast<float>(extent.height), 0, 1);
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, vk::Rect2D({0, 0}, extent));
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

void Renderer::recordSkybox(Vec3 cameraPosition, Vec3 cameraTarget,
                            std::vector<DrawCommand>& drawCommands)
{
    Vec3 worldUp{0.0f, 1.0f, 0.0f};
    Vec3 forward = Vec3::normalise(cameraTarget - cameraPosition);
    Vec3 right = Vec3::normalise(Vec3::crossProduct(forward, worldUp));
    Vec3 up = Vec3::crossProduct(right, forward);

    constexpr float skyboxFov = 45.0f * deg_to_rad;
    auto extent = swapchain_.extent();
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    float tanHalfFov = std::tan(skyboxFov * 0.5f);

    SkyboxUBO data{};
    data.cameraForward[0] = forward.x();
    data.cameraForward[1] = forward.y();
    data.cameraForward[2] = forward.z();
    data.cameraRight[0] = right.x();
    data.cameraRight[1] = right.y();
    data.cameraRight[2] = right.z();
    data.cameraUp[0] = up.x();
    data.cameraUp[1] = up.y();
    data.cameraUp[2] = up.z();
    data.viewParams[0] = tanHalfFov;
    data.viewParams[1] = aspect;
    std::memcpy(skyboxUbo_.mapped[currentFrame_], &data, sizeof(data));

    DrawCommand dc;
    dc.vertexBuffer = NullBuffer;
    dc.indexBuffer = skyboxIndexBuffer_;
    dc.indexCount = 3;
    dc.descriptorSet = skyboxDescSets_[currentFrame_];
    dc.pipeline = skyboxPipelineHandle_;
    drawCommands.push_back(dc);
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

    swapchain_.recreate(device_, display);
    forwardPass_.createForwardFramebuffers(device_, swapchain_);

    frame_.createRenderFinishedSemaphores(swapchain_.images().size());
}

} // namespace fire_engine
