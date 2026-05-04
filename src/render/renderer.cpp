#include <fire_engine/render/renderer.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include <fire_engine/graphics/image.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/render/environment_precompute.hpp>
#include <fire_engine/render/render_context.hpp>
#include <fire_engine/render/swapchain.hpp>
#include <fire_engine/render/ubo.hpp>
#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

Renderer::Renderer(const Window& window, std::string environmentPath)
    : device_(window),
      swapchain_(device_, window),
      forwardPass_(RenderPass::createForward(device_)),
      forwardTransmissionPass_(RenderPass::createForwardTransmission(device_)),
      pipelineOpaque_(device_, Pipeline::forwardConfig(forwardPass_.renderPass())),
      pipelineOpaqueDoubleSided_(device_,
                                 Pipeline::forwardDoubleSidedConfig(forwardPass_.renderPass())),
      pipelineBlend_(device_, Pipeline::forwardBlendConfig(forwardPass_.renderPass())),
      skyboxPipeline_(device_, Pipeline::skyboxConfig(forwardPass_.renderPass())),
      frame_(device_, swapchain_),
      resources_(device_, pipelineOpaque_),
      postProcessing_(device_, swapchain_, resources_),
      shadows_(device_, resources_),
      environmentPath_(std::move(environmentPath))
{
    swapchain_.createDepthResources(device_);
    forwardPass_.createForwardFramebuffer(
        device_, resources_.vulkanImageView(postProcessing_.offscreenColourTarget()),
        swapchain_.depthView(), swapchain_.extent());
    forwardTransmissionPass_.createForwardFramebuffer(
        device_, resources_.vulkanImageView(postProcessing_.offscreenColourTarget()),
        swapchain_.depthView(), swapchain_.extent());
    rebuildSceneColorChain(swapchain_.extent());
    forwardOpaqueHandle_ =
        resources_.registerPipeline(pipelineOpaque_.pipeline(), pipelineOpaque_.pipelineLayout());
    forwardOpaqueDoubleSidedHandle_ = resources_.registerPipeline(
        pipelineOpaqueDoubleSided_.pipeline(), pipelineOpaqueDoubleSided_.pipelineLayout());
    forwardBlendHandle_ =
        resources_.registerPipeline(pipelineBlend_.pipeline(), pipelineBlend_.pipelineLayout());
    skyboxPipelineHandle_ =
        resources_.registerPipeline(skyboxPipeline_.pipeline(), skyboxPipeline_.pipelineLayout());
    skyboxUbo_ = resources_.createMappedUniformBuffers(sizeof(SkyboxUBO));
    std::array<uint16_t, 3> skyboxIndices{0, 1, 2};
    skyboxIndexBuffer_ = resources_.createIndexBuffer(skyboxIndices);

    lightUbo_ = resources_.createMappedUniformBuffers(sizeof(LightUBO));
    resources_.lightBuffers(lightUbo_.buffers);
    EnvironmentPrecompute environmentPrecompute{device_, resources_, environmentPath_};
    environmentPrecompute.create(skyboxPipeline_.descriptorSetLayout(), skyboxUbo_,
                                 sizeof(SkyboxUBO), lightUbo_, sizeof(LightUBO));
    skyboxCubemapHandle_ = environmentPrecompute.skyboxCubemap();
    irradianceCubemapHandle_ = environmentPrecompute.irradianceCubemap();
    prefilteredCubemapHandle_ = environmentPrecompute.prefilteredCubemap();
    brdfLutHandle_ = environmentPrecompute.brdfLut();
    skyboxDescSets_ = environmentPrecompute.skyboxDescriptorSets();
    resources_.irradianceMap(irradianceCubemapHandle_);
    resources_.prefilteredMap(prefilteredCubemapHandle_);
    resources_.brdfLut(brdfLutHandle_);
    imagesInFlight_.assign(swapchain_.images().size(), vk::Fence{});
}

void Renderer::updateLightData(Vec3 cameraPosition, Vec3 cameraTarget, float aspect,
                               std::span<const Lighting> lights)
{
    // Pick a primary directional for CSM. First directional in the gather
    // order wins. If no directional is in the scene, the cascade fit still
    // runs against a sane default direction (light contributes nothing
    // because lightCount == 0 unless there are non-directionals).
    const Lighting* primaryDirectional = nullptr;
    for (const auto& l : lights)
    {
        if (l.type == 0)
        {
            primaryDirectional = &l;
            break;
        }
    }
    const Vec3 lightDir = primaryDirectional != nullptr
                              ? Vec3::normalise(Vec3{-primaryDirectional->worldDirection.x(),
                                                     -primaryDirectional->worldDirection.y(),
                                                     -primaryDirectional->worldDirection.z()})
                              : Vec3::normalise(Vec3{-1.0f, 1.0f, -1.0f});

    // Camera basis + light basis (shared by every cascade fit).
    const float tanHalfFov = std::tan(cameraFovRadians * 0.5f);
    const Vec3 forward = Vec3::normalise(cameraTarget - cameraPosition);
    const Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const Vec3 right = Vec3::normalise(Vec3::crossProduct(forward, worldUp));
    const Vec3 camUp = Vec3::crossProduct(right, forward);

    Vec3 lightUp = worldUp;
    if (std::abs(Vec3::dotProduct(lightDir, lightUp)) > 0.99f)
    {
        lightUp = Vec3{0.0f, 0.0f, 1.0f};
    }
    const Vec3 lightRight = Vec3::normalise(Vec3::crossProduct(lightDir, lightUp));
    const Vec3 lightUpOrtho = Vec3::crossProduct(lightRight, lightDir);
    const float shadowMapExtentF = static_cast<float>(shadowMapExtent);

    // Bounding-sphere fit for a single sub-frustum slice. Matches the Stage 1
    // fit — extracted so CSM can reuse it per cascade.
    auto fitCascade = [&](float sliceNear, float sliceFar) -> Mat4
    {
        const float nearH = tanHalfFov * sliceNear;
        const float nearW = nearH * aspect;
        const float farH = tanHalfFov * sliceFar;
        const float farW = farH * aspect;

        const Vec3 sliceNearCentre = cameraPosition + forward * sliceNear;
        const Vec3 sliceFarCentre = cameraPosition + forward * sliceFar;

        const std::array<Vec3, 8> corners{sliceNearCentre - right * nearW - camUp * nearH,
                                          sliceNearCentre + right * nearW - camUp * nearH,
                                          sliceNearCentre + right * nearW + camUp * nearH,
                                          sliceNearCentre - right * nearW + camUp * nearH,
                                          sliceFarCentre - right * farW - camUp * farH,
                                          sliceFarCentre + right * farW - camUp * farH,
                                          sliceFarCentre + right * farW + camUp * farH,
                                          sliceFarCentre - right * farW + camUp * farH};

        Vec3 frustumCentre{0.0f, 0.0f, 0.0f};
        for (const auto& c : corners)
        {
            frustumCentre += c;
        }
        frustumCentre /= 8.0f;

        float radius = 0.0f;
        for (const auto& c : corners)
        {
            radius = std::max(radius, (c - frustumCentre).magnitude());
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        const float worldPerTexel = (2.0f * radius) / shadowMapExtentF;
        const float centreU = Vec3::dotProduct(frustumCentre, lightRight);
        const float centreV = Vec3::dotProduct(frustumCentre, lightUpOrtho);
        const float centreW = Vec3::dotProduct(frustumCentre, lightDir);
        const float snappedU = std::floor(centreU / worldPerTexel) * worldPerTexel;
        const float snappedV = std::floor(centreV / worldPerTexel) * worldPerTexel;
        const Vec3 snappedCentre =
            lightRight * snappedU + lightUpOrtho * snappedV + lightDir * centreW;

        const Vec3 lightPos = snappedCentre - lightDir * (radius + shadowDepthBackExtend);
        const Mat4 lightView = Mat4::lookAt(lightPos, snappedCentre, lightUpOrtho);
        const Mat4 lightProj = Mat4::ortho(-radius, radius, -radius, radius, 0.0f,
                                           2.0f * radius + 2.0f * shadowDepthBackExtend);
        return lightProj * lightView;
    };

    // Log-uniform cascade splits (λ=0.5) — Practical Split Scheme. Keeps close
    // cascades small for near-camera detail while still covering shadowFarPlane.
    float splits[shadowCascadeCount];
    for (uint32_t i = 0; i < shadowCascadeCount; ++i)
    {
        const float p = static_cast<float>(i + 1) / static_cast<float>(shadowCascadeCount);
        const float linear = cameraNearPlane + (shadowFarPlane - cameraNearPlane) * p;
        const float logSplit = cameraNearPlane * std::pow(shadowFarPlane / cameraNearPlane, p);
        splits[i] = 0.5f * (linear + logSplit);
    }

    Mat4 cascadeViewProj[shadowCascadeCount];
    float sliceNear = cameraNearPlane;
    for (uint32_t i = 0; i < shadowCascadeCount; ++i)
    {
        cascadeViewProj[i] = fitCascade(sliceNear, splits[i]);
        sliceNear = splits[i];
    }
    for (uint32_t i = 0; i < shadowCascadeCount; ++i)
    {
        cascadeViewProjs_[i] = cascadeViewProj[i];
    }

    LightUBO lightData{};
    for (uint32_t i = 0; i < shadowCascadeCount; ++i)
    {
        lightData.cascadeViewProj[i] = cascadeViewProj[i];
        lightData.cascadeSplits[i] = splits[i];
    }

    // Pack lights into the UBO array. The primary directional (CSM source)
    // goes first so the shader can branch on i==0 for the shadow lookup.
    int slot = 0;
    auto pack = [&](const Lighting& L)
    {
        if (slot >= MAX_LIGHTS)
        {
            return;
        }
        LightData& dst = lightData.lights[slot++];
        dst.position[0] = L.worldPosition.x();
        dst.position[1] = L.worldPosition.y();
        dst.position[2] = L.worldPosition.z();
        dst.position[3] = static_cast<float>(L.type);
        dst.direction[0] = L.worldDirection.x();
        dst.direction[1] = L.worldDirection.y();
        dst.direction[2] = L.worldDirection.z();
        dst.direction[3] = L.range;
        dst.colour[0] = L.colour.r();
        dst.colour[1] = L.colour.g();
        dst.colour[2] = L.colour.b();
        dst.colour[3] = L.intensity;
        dst.cone[0] = L.innerConeCos;
        dst.cone[1] = L.outerConeCos;
    };
    if (primaryDirectional != nullptr)
    {
        pack(*primaryDirectional);
    }
    for (const auto& L : lights)
    {
        if (&L == primaryDirectional)
        {
            continue;
        }
        pack(L);
    }
    lightData.lightCount = slot;
    uint32_t mipLevels = prefilteredCubemapHandle_ != NullTexture
                             ? resources_.textureMipLevels(prefilteredCubemapHandle_)
                             : 1u;
    lightData.iblParams[0] = static_cast<float>(mipLevels > 0 ? mipLevels - 1 : 0);
    lightData.iblParams[1] = diffuseIblStrength;
    lightData.iblParams[2] = specularIblStrength;
    lightData.shadowParams[0] = shadowMinBias;
    lightData.shadowParams[1] = shadowSlopeBias;
    lightData.shadowParams[2] = shadowFilterRadius;
    lightData.shadowParams[3] = pcssLightSize;
    lightData.environmentParams[0] = skyboxIntensity;
    lightData.environmentParams[3] = cascadeDebugTint_ ? 1.0f : 0.0f;
    std::memcpy(lightUbo_.mapped[currentFrame_], &lightData, sizeof(lightData));
}

Renderer::DrawBuckets Renderer::buildDrawBuckets(const std::vector<DrawCommand>& drawCommands) const
{
    DrawBuckets buckets;
    buckets.opaque.reserve(drawCommands.size());
    for (const auto& dc : drawCommands)
    {
        if (dc.pipeline == shadows_.pipelineHandle())
        {
            buckets.shadow.push_back(dc);
        }
        else if (dc.pipeline == forwardBlendHandle_)
        {
            buckets.blend.push_back(dc);
        }
        else if (dc.transmissive)
        {
            // KHR_materials_transmission F3 — defer to the second forward
            // sub-pass so the fragment shader can sample sceneColor.
            buckets.transmissive.push_back(dc);
        }
        else
        {
            buckets.opaque.push_back(dc);
        }
    }

    std::sort(buckets.blend.begin(), buckets.blend.end(),
              [](const DrawCommand& a, const DrawCommand& b) { return a.sortDepth > b.sortDepth; });
    std::sort(buckets.transmissive.begin(), buckets.transmissive.end(),
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

void Renderer::recordForwardPass(vk::CommandBuffer cmd, const DrawBuckets& buckets)
{
    beginRenderPass(cmd);
    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    recordDrawBucket(cmd, buckets.opaque, lastBoundPipeline);
    recordDrawBucket(cmd, buckets.blend, lastBoundPipeline);
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

    const auto extent = swapchain_.extent();
    const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    auto lights = scene.gatherLights();
    updateLightData(cameraPosition, cameraTarget, aspect, lights);

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
                      shadows_.pipelineHandle(),
                      cascadeViewProjs_};
    scene.render(ctx);

    DrawBuckets buckets = buildDrawBuckets(drawCommands);
    shadows_.recordPass(cmd, buckets.shadow);
    recordForwardPass(cmd, buckets);
    if (!buckets.transmissive.empty())
    {
        // KHR_materials_transmission F3 — capture the post-opaque HDR target
        // into the sceneColor mip chain, then render transmissive draws on
        // top so their fragment shader can sample sceneColor for screen-space
        // refraction.
        recordSceneColorCapture(cmd);
        recordForwardTransmissionPass(cmd, buckets);
    }
    postProcessing_.transitionOffscreenForSampling(cmd);
    postProcessing_.recordBloomPasses(cmd);
    postProcessing_.recordPostProcessPass(cmd, *imageIndex, currentFrame_);

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
    std::array<vk::ClearValue, 2> clears = {
        vk::ClearValue{.color = vk::ClearColorValue{.float32 = {{0.02f, 0.02f, 0.02f, 1.0f}}}},
        vk::ClearValue{.depthStencil = vk::ClearDepthStencilValue{.depth = 1.0f, .stencil = 0}},
    };

    vk::Rect2D renderArea{
        .offset = vk::Offset2D{.x = 0, .y = 0},
        .extent = extent,
    };
    vk::RenderPassBeginInfo rpBegin{
        .renderPass = forwardPass_.renderPass(),
        .framebuffer = forwardPass_.framebuffer(0),
        .renderArea = renderArea,
        .clearValueCount = static_cast<uint32_t>(clears.size()),
        .pClearValues = clears.data(),
    };

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, renderArea);
}

void Renderer::submitAndPresent(Window& display, vk::CommandBuffer cmd, uint32_t imageIndex)
{
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    auto imageAvail = frame_.imageAvailable(currentFrame_);
    auto renderDone = frame_.renderFinished(imageIndex);
    vk::SubmitInfo si{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvail,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderDone,
    };

    device_.graphicsQueue().submit(si, frame_.inFlightFence(currentFrame_));

    auto swapchain = swapchain_.swapchain();
    vk::PresentInfoKHR pi{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderDone,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };

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

    constexpr float skyboxFov = cameraFovRadians;
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

void Renderer::rebuildSceneColorChain(vk::Extent2D extent)
{
    if (sceneColorHandle_ != NullTexture)
    {
        resources_.releaseTexture(sceneColorHandle_);
        sceneColorHandle_ = NullTexture;
    }
    const uint32_t maxDim = std::max(extent.width, extent.height);
    sceneColorMipLevels_ = 1u;
    while ((maxDim >> sceneColorMipLevels_) > 0)
    {
        ++sceneColorMipLevels_;
    }
    sceneColorHandle_ =
        resources_.createSceneColorTarget(extent.width, extent.height, sceneColorMipLevels_);
    resources_.sceneColor(sceneColorHandle_);
}

void Renderer::recordSceneColorCapture(vk::CommandBuffer cmd)
{
    auto extent = swapchain_.extent();
    auto hdrImage = resources_.vulkanImage(postProcessing_.offscreenColourTarget());
    auto sceneImage = resources_.vulkanImage(sceneColorHandle_);

    auto colourRange = [](uint32_t baseMip, uint32_t levels)
    {
        return vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = baseMip,
            .levelCount = levels,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
    };

    // 1. HDR target (currently in ShaderReadOnlyOptimal after forward pass) →
    //    TransferSrcOptimal so we can blit it.
    vk::ImageMemoryBarrier hdrToSrc{
        .srcAccessMask = vk::AccessFlagBits::eShaderRead,
        .dstAccessMask = vk::AccessFlagBits::eTransferRead,
        .oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = hdrImage,
        .subresourceRange = colourRange(0, 1),
    };
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, hdrToSrc);

    // 2. sceneColor mips were left in ShaderReadOnlyOptimal at creation (and
    //    by the previous frame's capture). Transition them all to
    //    TransferDstOptimal so the blit chain can write each level.
    vk::ImageMemoryBarrier sceneToDst{
        .srcAccessMask = vk::AccessFlagBits::eShaderRead,
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = sceneImage,
        .subresourceRange = colourRange(0, sceneColorMipLevels_),
    };
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader,
                        vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, sceneToDst);

    // 3. Blit HDR (mip 0 only) → sceneColor mip 0 at full extent.
    vk::ImageBlit blit0{
        .srcSubresource = vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                     .mipLevel = 0,
                                                     .baseArrayLayer = 0,
                                                     .layerCount = 1},
        .srcOffsets =
            std::array<vk::Offset3D, 2>{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                        vk::Offset3D{.x = static_cast<int32_t>(extent.width),
                                                     .y = static_cast<int32_t>(extent.height),
                                                     .z = 1}},
        .dstSubresource = vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                     .mipLevel = 0,
                                                     .baseArrayLayer = 0,
                                                     .layerCount = 1},
        .dstOffsets =
            std::array<vk::Offset3D, 2>{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                        vk::Offset3D{.x = static_cast<int32_t>(extent.width),
                                                     .y = static_cast<int32_t>(extent.height),
                                                     .z = 1}},
    };
    cmd.blitImage(hdrImage, vk::ImageLayout::eTransferSrcOptimal, sceneImage,
                  vk::ImageLayout::eTransferDstOptimal, blit0, vk::Filter::eLinear);

    // 4. Generate the rest of the mip chain by halving each dimension.
    int32_t mipW = static_cast<int32_t>(extent.width);
    int32_t mipH = static_cast<int32_t>(extent.height);
    for (uint32_t i = 1; i < sceneColorMipLevels_; ++i)
    {
        // Source mip (i - 1): TransferDst → TransferSrc.
        vk::ImageMemoryBarrier srcReady{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = sceneImage,
            .subresourceRange = colourRange(i - 1, 1),
        };
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, srcReady);

        const int32_t nextW = std::max(1, mipW >> 1);
        const int32_t nextH = std::max(1, mipH >> 1);
        vk::ImageBlit blit{
            .srcSubresource =
                vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                           .mipLevel = i - 1,
                                           .baseArrayLayer = 0,
                                           .layerCount = 1},
            .srcOffsets = std::array<vk::Offset3D, 2>{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                                      vk::Offset3D{.x = mipW, .y = mipH, .z = 1}},
            .dstSubresource =
                vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                           .mipLevel = i,
                                           .baseArrayLayer = 0,
                                           .layerCount = 1},
            .dstOffsets = std::array<vk::Offset3D, 2>{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                                      vk::Offset3D{.x = nextW, .y = nextH, .z = 1}},
        };
        cmd.blitImage(sceneImage, vk::ImageLayout::eTransferSrcOptimal, sceneImage,
                      vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

        // Source mip (i - 1) → ShaderReadOnly so the shader can sample any level.
        vk::ImageMemoryBarrier srcDone{
            .srcAccessMask = vk::AccessFlagBits::eTransferRead,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = sceneImage,
            .subresourceRange = colourRange(i - 1, 1),
        };
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, srcDone);

        mipW = nextW;
        mipH = nextH;
    }

    // Final mip is still in TransferDstOptimal — transition to ShaderReadOnly.
    vk::ImageMemoryBarrier lastMipDone{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = sceneImage,
        .subresourceRange = colourRange(sceneColorMipLevels_ - 1, 1),
    };
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, lastMipDone);

    // 5. HDR target back to ColorAttachmentOptimal so the second forward pass
    //    can render onto it (loadOp=eLoad keeps existing contents).
    vk::ImageMemoryBarrier hdrBack{
        .srcAccessMask = vk::AccessFlagBits::eTransferRead,
        .dstAccessMask =
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = hdrImage,
        .subresourceRange = colourRange(0, 1),
    };
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, hdrBack);
}

void Renderer::recordForwardTransmissionPass(vk::CommandBuffer cmd, const DrawBuckets& buckets)
{
    if (buckets.transmissive.empty())
    {
        return;
    }

    auto extent = swapchain_.extent();
    vk::Rect2D renderArea{.offset = vk::Offset2D{.x = 0, .y = 0}, .extent = extent};
    vk::RenderPassBeginInfo begin{
        .renderPass = forwardTransmissionPass_.renderPass(),
        .framebuffer = forwardTransmissionPass_.framebuffer(0),
        .renderArea = renderArea,
    };
    cmd.beginRenderPass(begin, vk::SubpassContents::eInline);
    cmd.setViewport(0, vk::Viewport{.x = 0.0f,
                                    .y = 0.0f,
                                    .width = static_cast<float>(extent.width),
                                    .height = static_cast<float>(extent.height),
                                    .minDepth = 0.0f,
                                    .maxDepth = 1.0f});
    cmd.setScissor(0, renderArea);
    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    recordDrawBucket(cmd, buckets.transmissive, lastBoundPipeline);
    cmd.endRenderPass();
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
    postProcessing_.recreate();
    forwardPass_.createForwardFramebuffer(
        device_, resources_.vulkanImageView(postProcessing_.offscreenColourTarget()),
        swapchain_.depthView(), swapchain_.extent());
    forwardTransmissionPass_.createForwardFramebuffer(
        device_, resources_.vulkanImageView(postProcessing_.offscreenColourTarget()),
        swapchain_.depthView(), swapchain_.extent());
    rebuildSceneColorChain(swapchain_.extent());
    frame_.createRenderFinishedSemaphores(swapchain_.images().size());
    imagesInFlight_.assign(swapchain_.images().size(), vk::Fence{});
}

} // namespace fire_engine
