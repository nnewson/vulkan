#include <fire_engine/renderer/renderer.hpp>

#include <fire_engine/core/system.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject
{
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    alignas(16) float cameraPos[4];
};

Renderer::Renderer(const Window& window)
    : device_(window),
      swapchain_(device_, window),
      pipeline_(device_, swapchain_),
      frame_(device_, swapchain_, pipeline_)
{
    swapchain_.createDepthResources(device_);
    swapchain_.createFramebuffers(pipeline_.renderPass());
}

void Renderer::drawFrame(Window& display, Vec3 cameraPos, Vec3 cameraTarget)
{
    auto dev = device_.device();

    (void)dev.waitForFences(frame_.inFlightFence(currentFrame_), vk::True, UINT64_MAX);

    auto [acquireResult, imageIndex] =
        dev.acquireNextImageKHR(swapchain_.swapchain(), UINT64_MAX,
                                frame_.imageAvailable(currentFrame_));
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain(display);
        return;
    }
    if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("failed to acquire swap chain image");

    dev.resetFences(frame_.inFlightFence(currentFrame_));

    updateUniformBuffer(cameraPos, cameraTarget);

    auto cmd = frame_.commandBuffer(currentFrame_);
    cmd.reset();
    recordCommandBuffer(cmd, imageIndex);

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

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex)
{
    auto extent = swapchain_.extent();

    cmd.begin(vk::CommandBufferBeginInfo{});

    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue(std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f});
    clears[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo rpBegin(pipeline_.renderPass(),
                                    swapchain_.framebuffers()[imageIndex],
                                    vk::Rect2D({0, 0}, extent), clears);

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_.pipeline());
    cmd.bindVertexBuffers(0, frame_.vertexBuffer(), {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(frame_.indexBuffer(), 0, vk::IndexType::eUint16);
    auto descSet = frame_.descriptorSet(currentFrame_);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_.pipelineLayout(), 0,
                           descSet, {});
    cmd.drawIndexed(static_cast<uint32_t>(frame_.renderData().indices.size()), 1, 0, 0, 0);
    cmd.endRenderPass();
    cmd.end();
}

void Renderer::updateUniformBuffer(Vec3 cameraPos, Vec3 cameraTarget)
{
    static auto startTime = System::getTime();
    float t = static_cast<float>(System::getTime() - startTime);

    auto extent = swapchain_.extent();

    UniformBufferObject ubo{};
    ubo.model = Mat4::rotateY(t * 1.0f) * Mat4::rotateX(t * 0.5f);
    ubo.view = Mat4::lookAt(cameraPos, cameraTarget, {0, 1, 0});
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = cameraPos.x();
    ubo.cameraPos[1] = cameraPos.y();
    ubo.cameraPos[2] = cameraPos.z();
    ubo.cameraPos[3] = 0.0f;

    memcpy(frame_.uniformMapped(currentFrame_), &ubo, sizeof(ubo));
}

void Renderer::recreateSwapchain(const Window& display)
{
    int w = 0, h = 0;
    glfwGetFramebufferSize(display.getWindow(), &w, &h);
    while (w == 0 || h == 0)
    {
        glfwGetFramebufferSize(display.getWindow(), &w, &h);
        glfwWaitEvents();
    }
    waitIdle();

    frame_.destroyRenderFinishedSemaphores();

    swapchain_.recreate(device_, display, pipeline_.renderPass());

    frame_.createRenderFinishedSemaphores(swapchain_.images().size());
}

} // namespace fire_engine
