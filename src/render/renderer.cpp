#include <fire_engine/render/renderer.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <tuple>

#include <fire_engine/graphics/image.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/render/render_context.hpp>
#include <fire_engine/render/swapchain.hpp>
#include <fire_engine/render/ubo.hpp>
#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

namespace
{

// Calibrated against the current ACES post-process so direct light stays crisp without washing out
// midtones.
constexpr float directionalLightIntensity = 0.85f;
constexpr const char* skyboxFilename = "skybox.hdr";

#ifndef FIRE_ENGINE_SOURCE_ASSET_DIR
#define FIRE_ENGINE_SOURCE_ASSET_DIR "assets"
#endif

#ifndef FIRE_ENGINE_BUILD_ASSET_DIR
#define FIRE_ENGINE_BUILD_ASSET_DIR "."
#endif

[[nodiscard]] std::string resolveSkyboxPath()
{
    namespace fs = std::filesystem;

    const std::array<fs::path, 4> candidates = {
        fs::path(skyboxFilename),
        fs::path(FIRE_ENGINE_BUILD_ASSET_DIR) / skyboxFilename,
        fs::path(FIRE_ENGINE_SOURCE_ASSET_DIR) / skyboxFilename,
        fs::path("assets") / skyboxFilename,
    };

    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return candidate.string();
        }
    }

    throw std::runtime_error(
        "Failed to locate skybox asset. Tried: skybox.hdr, " +
        (fs::path(FIRE_ENGINE_BUILD_ASSET_DIR) / skyboxFilename).string() + ", " +
        (fs::path(FIRE_ENGINE_SOURCE_ASSET_DIR) / skyboxFilename).string() + ", assets/skybox.hdr");
}

[[nodiscard]] Vec3 directionForCubemapFace(uint32_t face, float u, float v)
{
    switch (face)
    {
    case 0:
        return Vec3::normalise({1.0f, v, -u});
    case 1:
        return Vec3::normalise({-1.0f, v, u});
    case 2:
        return Vec3::normalise({u, 1.0f, -v});
    case 3:
        return Vec3::normalise({u, -1.0f, v});
    case 4:
        return Vec3::normalise({u, v, 1.0f});
    default:
        return Vec3::normalise({-u, v, -1.0f});
    }
}

[[nodiscard]] std::array<float, 4> sampleEquirectangular(const Image& image, const Vec3& dir)
{
    const float* pixels = image.dataf();
    if (pixels == nullptr)
    {
        throw std::runtime_error("skybox.hdr did not decode as HDR float data");
    }

    float phi = std::atan2(dir.z(), dir.x());
    float theta = std::asin(std::clamp(dir.y(), -1.0f, 1.0f));
    float u = 0.5f + phi / (2.0f * pi);
    float v = 0.5f - theta / pi;

    u -= std::floor(u);
    v = std::clamp(v, 0.0f, 1.0f);

    float x = u * static_cast<float>(image.width() - 1);
    float y = v * static_cast<float>(image.height() - 1);

    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = std::min(x0 + 1, image.width() - 1);
    int y1 = std::min(y0 + 1, image.height() - 1);

    float tx = x - static_cast<float>(x0);
    float ty = y - static_cast<float>(y0);

    auto samplePixel = [&](int px, int py, int channel)
    {
        std::size_t index =
            (static_cast<std::size_t>(py) * image.width() + static_cast<std::size_t>(px)) * 4 +
            static_cast<std::size_t>(channel);
        return pixels[index];
    };

    std::array<float, 4> result{};
    for (int channel = 0; channel < 4; ++channel)
    {
        float c00 = samplePixel(x0, y0, channel);
        float c10 = samplePixel(x1, y0, channel);
        float c01 = samplePixel(x0, y1, channel);
        float c11 = samplePixel(x1, y1, channel);
        float c0 = c00 + (c10 - c00) * tx;
        float c1 = c01 + (c11 - c01) * tx;
        result[static_cast<std::size_t>(channel)] = c0 + (c1 - c0) * ty;
    }
    return result;
}

} // namespace

Renderer::Renderer(const Window& window)
    : device_(window),
      swapchain_(device_, window),
      forwardPass_(RenderPass::createForward(device_)),
      shadowPass_(RenderPass::createShadow(device_)),
      postProcessPass_(RenderPass::createPostProcess(device_, swapchain_)),
      pipelineOpaque_(device_, Pipeline::forwardConfig(forwardPass_.renderPass())),
      pipelineOpaqueDoubleSided_(device_,
                                 Pipeline::forwardDoubleSidedConfig(forwardPass_.renderPass())),
      pipelineBlend_(device_, Pipeline::forwardBlendConfig(forwardPass_.renderPass())),
      skyboxPipeline_(device_, Pipeline::skyboxConfig(forwardPass_.renderPass())),
      shadowPipeline_(device_, Pipeline::shadowConfig(shadowPass_.renderPass())),
      postProcessPipeline_(device_, Pipeline::postProcessConfig(postProcessPass_.renderPass())),
      frame_(device_, swapchain_),
      resources_(device_, pipelineOpaque_)
{
    swapchain_.createDepthResources(device_);
    offscreenColourHandle_ = resources_.createOffscreenColourTarget(swapchain_.extent());
    forwardPass_.createForwardFramebuffer(device_,
                                          resources_.vulkanImageView(offscreenColourHandle_),
                                          swapchain_.depthView(), swapchain_.extent());
    postProcessPass_.createPostProcessFramebuffers(device_, swapchain_);
    forwardOpaqueHandle_ =
        resources_.registerPipeline(pipelineOpaque_.pipeline(), pipelineOpaque_.pipelineLayout());
    forwardOpaqueDoubleSidedHandle_ = resources_.registerPipeline(
        pipelineOpaqueDoubleSided_.pipeline(), pipelineOpaqueDoubleSided_.pipelineLayout());
    forwardBlendHandle_ =
        resources_.registerPipeline(pipelineBlend_.pipeline(), pipelineBlend_.pipelineLayout());
    skyboxPipelineHandle_ =
        resources_.registerPipeline(skyboxPipeline_.pipeline(), skyboxPipeline_.pipelineLayout());
    shadowPipelineHandle_ =
        resources_.registerPipeline(shadowPipeline_.pipeline(), shadowPipeline_.pipelineLayout());
    postProcessPipelineHandle_ = resources_.registerPipeline(postProcessPipeline_.pipeline(),
                                                             postProcessPipeline_.pipelineLayout());
    skyboxUbo_ = resources_.createMappedUniformBuffers(sizeof(SkyboxUBO));
    std::array<uint16_t, 3> skyboxIndices{0, 1, 2};
    skyboxIndexBuffer_ = resources_.createIndexBuffer(skyboxIndices);
    std::array<uint16_t, 3> postProcessIndices{0, 1, 2};
    postProcessIndexBuffer_ = resources_.createIndexBuffer(postProcessIndices);
    postProcessDescSets_ = resources_.createSingleImageSamplerDescriptors(
        postProcessPipeline_.descriptorSetLayout(), offscreenColourHandle_);

    lightUbo_ = resources_.createMappedUniformBuffers(sizeof(LightUBO));
    resources_.lightBuffers(lightUbo_.buffers);

    shadowMapHandle_ = resources_.createShadowMap(shadowMapSize_);
    shadowColourHandle_ = resources_.createShadowColourAttachment(shadowMapSize_);
    shadowPass_.createShadowFramebuffer(device_, resources_.vulkanImageView(shadowColourHandle_),
                                        resources_.vulkanImageView(shadowMapHandle_),
                                        shadowMapSize_);
    resources_.shadowDescriptorSetLayout(shadowPipeline_.descriptorSetLayout());
    resources_.shadowMap(shadowMapHandle_);
    createSkyboxEnvironment();
    imagesInFlight_.assign(swapchain_.images().size(), vk::Fence{});
}

void Renderer::createSkyboxEnvironment()
{
    std::string skyboxPath = resolveSkyboxPath();
    Image hdr = Image::load_from_file(skyboxPath);
    if (hdr.pixelType() != ImagePixelType::Float32)
    {
        throw std::runtime_error("Expected HDR float skybox image: " + skyboxPath);
    }

    std::vector<float> cubemapPixels(static_cast<std::size_t>(skyboxCubemapExtent_) *
                                     skyboxCubemapExtent_ * 4 * 6);

    for (uint32_t face = 0; face < 6; ++face)
    {
        for (uint32_t y = 0; y < skyboxCubemapExtent_; ++y)
        {
            for (uint32_t x = 0; x < skyboxCubemapExtent_; ++x)
            {
                float u = (2.0f * (static_cast<float>(x) + 0.5f) /
                           static_cast<float>(skyboxCubemapExtent_)) -
                          1.0f;
                float v = (2.0f * (static_cast<float>(y) + 0.5f) /
                           static_cast<float>(skyboxCubemapExtent_)) -
                          1.0f;
                Vec3 dir = directionForCubemapFace(face, u, -v);
                auto sample = sampleEquirectangular(hdr, dir);

                std::size_t index =
                    (static_cast<std::size_t>(face) * skyboxCubemapExtent_ * skyboxCubemapExtent_ +
                     static_cast<std::size_t>(y) * skyboxCubemapExtent_ +
                     static_cast<std::size_t>(x)) *
                    4;
                cubemapPixels[index + 0] = sample[0];
                cubemapPixels[index + 1] = sample[1];
                cubemapPixels[index + 2] = sample[2];
                cubemapPixels[index + 3] = 1.0f;
            }
        }
    }

    SamplerSettings sampler{};
    sampler.wrapS = WrapMode::ClampToEdge;
    sampler.wrapT = WrapMode::ClampToEdge;
    skyboxCubemapHandle_ =
        resources_.createCubemapTexture(cubemapPixels.data(), skyboxCubemapExtent_, sampler);
    skyboxDescSets_ = resources_.createUboImageSamplerDescriptors(
        skyboxPipeline_.descriptorSetLayout(), skyboxUbo_, sizeof(SkyboxUBO), skyboxCubemapHandle_);
}

void Renderer::updateLightData()
{
    Vec3 lightDir = Vec3::normalise({-1.0f, 1.0f, -1.0f});
    Vec3 lightPos = lightDir * 15.0f;
    Mat4 lightView = Mat4::lookAt(lightPos, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    Mat4 lightProj = Mat4::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    lightViewProj_ = lightProj * lightView;

    LightUBO lightData{};
    lightData.direction[0] = lightDir.x();
    lightData.direction[1] = lightDir.y();
    lightData.direction[2] = lightDir.z();
    lightData.direction[3] = 0.0f;
    lightData.colour[0] = 1.0f;
    lightData.colour[1] = 1.0f;
    lightData.colour[2] = 1.0f;
    lightData.colour[3] = directionalLightIntensity;
    lightData.lightViewProj = lightViewProj_;
    std::memcpy(lightUbo_.mapped[currentFrame_], &lightData, sizeof(lightData));
}

Renderer::DrawBuckets Renderer::buildDrawBuckets(const std::vector<DrawCommand>& drawCommands) const
{
    DrawBuckets buckets;
    buckets.opaque.reserve(drawCommands.size());
    for (const auto& dc : drawCommands)
    {
        if (dc.pipeline == shadowPipelineHandle_)
        {
            buckets.shadow.push_back(dc);
        }
        else if (dc.pipeline == forwardBlendHandle_)
        {
            buckets.blend.push_back(dc);
        }
        else
        {
            buckets.opaque.push_back(dc);
        }
    }

    std::sort(buckets.blend.begin(), buckets.blend.end(),
              [](const DrawCommand& a, const DrawCommand& b) { return a.sortDepth > b.sortDepth; });
    return buckets;
}

void Renderer::recordDrawBucket(vk::CommandBuffer cmd, const std::vector<DrawCommand>& bucket,
                                PipelineHandle& lastBoundPipeline) const
{
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
            cmd.bindVertexBuffers(0, resources_.vulkanBuffer(dc.vertexBuffer), {vk::DeviceSize{0}});
        }

        vk::IndexType indexType =
            dc.indexType == DrawIndexType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
        cmd.bindIndexBuffer(resources_.vulkanBuffer(dc.indexBuffer), 0, indexType);

        vk::DescriptorSet ds = resources_.vulkanDescriptorSet(dc.descriptorSet);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               resources_.vulkanPipelineLayout(dc.pipeline), 0, ds, {});
        cmd.drawIndexed(dc.indexCount, 1, 0, 0, 0);
    }
}

void Renderer::recordShadowPass(vk::CommandBuffer cmd, const std::vector<DrawCommand>& shadowDraws)
{
    std::array<vk::ClearValue, 2> shadowClears;
    shadowClears[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    shadowClears[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderPassBeginInfo shadowBegin(shadowPass_.renderPass(), shadowPass_.framebuffer(0),
                                        vk::Rect2D({0, 0}, {shadowMapSize_, shadowMapSize_}),
                                        shadowClears);
    cmd.beginRenderPass(shadowBegin, vk::SubpassContents::eInline);

    vk::Viewport shadowVp(0, 0, static_cast<float>(shadowMapSize_),
                          static_cast<float>(shadowMapSize_), 0, 1);
    cmd.setViewport(0, shadowVp);
    cmd.setScissor(0, vk::Rect2D({0, 0}, {shadowMapSize_, shadowMapSize_}));

    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    recordDrawBucket(cmd, shadowDraws, lastBoundPipeline);
    cmd.endRenderPass();
}

void Renderer::recordForwardPass(vk::CommandBuffer cmd, const DrawBuckets& buckets)
{
    beginRenderPass(cmd);
    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    recordDrawBucket(cmd, buckets.opaque, lastBoundPipeline);
    recordDrawBucket(cmd, buckets.blend, lastBoundPipeline);
    cmd.endRenderPass();
}

void Renderer::transitionOffscreenForSampling(vk::CommandBuffer cmd)
{
    vk::ImageMemoryBarrier offscreenReadyForSampling(
        vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        resources_.vulkanImage(offscreenColourHandle_),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                        offscreenReadyForSampling);
}

void Renderer::recordPostProcessPass(vk::CommandBuffer cmd, uint32_t imageIndex)
{
    auto extent = swapchain_.extent();
    vk::RenderPassBeginInfo ppBegin(postProcessPass_.renderPass(),
                                    postProcessPass_.framebuffer(imageIndex),
                                    vk::Rect2D({0, 0}, extent), {});
    cmd.beginRenderPass(ppBegin, vk::SubpassContents::eInline);

    vk::Viewport vp(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0,
                    1);
    cmd.setViewport(0, vp);
    cmd.setScissor(0, vk::Rect2D({0, 0}, extent));
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                     resources_.vulkanPipeline(postProcessPipelineHandle_));

    vk::DescriptorSet ppSet = resources_.vulkanDescriptorSet(postProcessDescSets_[currentFrame_]);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                           resources_.vulkanPipelineLayout(postProcessPipelineHandle_), 0, ppSet,
                           {});
    cmd.bindIndexBuffer(resources_.vulkanBuffer(postProcessIndexBuffer_), 0,
                        vk::IndexType::eUint16);
    cmd.drawIndexed(3, 1, 0, 0, 0);
    cmd.endRenderPass();
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

    // Directional light — fixed orthographic volume fitted around the origin.
    // Light position is the light direction scaled out so the frustum captures
    // the scene. The forward fragment shader consumes lightViewProj_ to project
    // fragments into shadow-map space.
    updateLightData();

    std::vector<DrawCommand> drawCommands;
    recordSkybox(cameraPosition, cameraTarget, drawCommands);

    AlphaPipelines pipelines{forwardOpaqueHandle_, forwardOpaqueDoubleSidedHandle_,
                             forwardBlendHandle_};
    RenderContext ctx{device_,
                      swapchain_,
                      frame_,
                      pipelineOpaque_,
                      cmd,
                      currentFrame_,
                      cameraPosition,
                      cameraTarget,
                      &drawCommands,
                      pipelines,
                      shadowPipelineHandle_,
                      lightViewProj_};
    scene.render(ctx);

    DrawBuckets buckets = buildDrawBuckets(drawCommands);
    recordShadowPass(cmd, buckets.shadow);
    recordForwardPass(cmd, buckets);
    transitionOffscreenForSampling(cmd);
    recordPostProcessPass(cmd, *imageIndex);

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

    vk::Fence currentFrameFence = frame_.inFlightFence(currentFrame_);
    if (imagesInFlight_[imageIndex])
    {
        (void)dev.waitForFences(imagesInFlight_[imageIndex], vk::True, UINT64_MAX);
    }

    dev.resetFences(currentFrameFence);
    imagesInFlight_[imageIndex] = currentFrameFence;
    return imageIndex;
}

void Renderer::beginRenderPass(vk::CommandBuffer cmd)
{
    auto extent = swapchain_.extent();
    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue(std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f});
    clears[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo rpBegin(forwardPass_.renderPass(), forwardPass_.framebuffer(0),
                                    vk::Rect2D({0, 0}, extent), clears);

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

    vk::Viewport viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height),
                          0, 1);
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
    dc.indexType = DrawIndexType::UInt16;
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
    resources_.releaseTexture(offscreenColourHandle_);
    offscreenColourHandle_ = resources_.createOffscreenColourTarget(swapchain_.extent());
    forwardPass_.createForwardFramebuffer(device_,
                                          resources_.vulkanImageView(offscreenColourHandle_),
                                          swapchain_.depthView(), swapchain_.extent());
    postProcessPass_.createPostProcessFramebuffers(device_, swapchain_);
    resources_.updateSingleImageSamplerDescriptors(postProcessDescSets_, offscreenColourHandle_);
    frame_.createRenderFinishedSemaphores(swapchain_.images().size());
    imagesInFlight_.assign(swapchain_.images().size(), vk::Fence{});
}

} // namespace fire_engine
