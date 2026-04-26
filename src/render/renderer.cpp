#include <fire_engine/render/renderer.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <stdexcept>

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

// Default HDR environment asset, resolved against the project's asset dirs.
// Tunable rendering values (extents, strengths, biases, etc.) live in
// render/constants.hpp.
constexpr const char* defaultEnvironmentFilename = "skybox.hdr";

#ifndef FIRE_ENGINE_SOURCE_ASSET_DIR
#define FIRE_ENGINE_SOURCE_ASSET_DIR "assets"
#endif

#ifndef FIRE_ENGINE_BUILD_ASSET_DIR
#define FIRE_ENGINE_BUILD_ASSET_DIR "."
#endif

[[nodiscard]] std::string resolveSkyboxPath(const std::string& requestedPath);
[[nodiscard]] Image loadEnvironmentImage(const std::string& requestedPath, const char* stageName);

[[nodiscard]] std::string resolveSkyboxPath(const std::string& requestedPath)
{
    namespace fs = std::filesystem;

    if (!requestedPath.empty())
    {
        fs::path requested{requestedPath};
        if (fs::exists(requested))
        {
            return requested.string();
        }

        const std::array<fs::path, 3> requestedCandidates = {
            fs::path(FIRE_ENGINE_BUILD_ASSET_DIR) / requested,
            fs::path(FIRE_ENGINE_SOURCE_ASSET_DIR) / requested,
            fs::path("assets") / requested,
        };

        for (const auto& candidate : requestedCandidates)
        {
            if (fs::exists(candidate))
            {
                return candidate.string();
            }
        }

        throw std::runtime_error("Failed to locate requested HDR environment asset: " +
                                 requested.string());
    }

    const std::array<fs::path, 4> candidates = {
        fs::path(defaultEnvironmentFilename),
        fs::path(FIRE_ENGINE_BUILD_ASSET_DIR) / defaultEnvironmentFilename,
        fs::path(FIRE_ENGINE_SOURCE_ASSET_DIR) / defaultEnvironmentFilename,
        fs::path("assets") / defaultEnvironmentFilename,
    };

    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return candidate.string();
        }
    }

    throw std::runtime_error(
        "Failed to locate HDR environment asset. Tried: " +
        fs::path(defaultEnvironmentFilename).string() + ", " +
        (fs::path(FIRE_ENGINE_BUILD_ASSET_DIR) / defaultEnvironmentFilename).string() + ", " +
        (fs::path(FIRE_ENGINE_SOURCE_ASSET_DIR) / defaultEnvironmentFilename).string() + ", " +
        (fs::path("assets") / defaultEnvironmentFilename).string());
}

[[nodiscard]] Image loadEnvironmentImage(const std::string& requestedPath, const char* stageName)
{
    try
    {
        std::string environmentPath = resolveSkyboxPath(requestedPath);
        Image hdr = Image::load_from_file(environmentPath);
        if (hdr.pixelType() != ImagePixelType::Float32)
        {
            throw std::runtime_error("decoded image is not HDR float data");
        }
        return hdr;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Environment bootstrap failed during ") + stageName +
                                 ": " + e.what());
    }
}

} // namespace

Renderer::Renderer(const Window& window, std::string environmentPath)
    : device_(window),
      swapchain_(device_, window),
      forwardPass_(RenderPass::createForward(device_)),
      shadowPass_(RenderPass::createShadow(device_)),
      postProcessPass_(RenderPass::createPostProcess(device_, swapchain_)),
      bloomDownPass_(RenderPass::createBloomDown(device_, vk::Format::eR16G16B16A16Sfloat)),
      bloomUpPass_(RenderPass::createBloomUp(device_, vk::Format::eR16G16B16A16Sfloat)),
      pipelineOpaque_(device_, Pipeline::forwardConfig(forwardPass_.renderPass())),
      pipelineOpaqueDoubleSided_(device_,
                                 Pipeline::forwardDoubleSidedConfig(forwardPass_.renderPass())),
      pipelineBlend_(device_, Pipeline::forwardBlendConfig(forwardPass_.renderPass())),
      skyboxPipeline_(device_, Pipeline::skyboxConfig(forwardPass_.renderPass())),
      shadowPipeline_(device_, Pipeline::shadowConfig(shadowPass_.renderPass())),
      postProcessPipeline_(device_, Pipeline::postProcessConfig(postProcessPass_.renderPass())),
      bloomDownsamplePipeline_(device_,
                               Pipeline::bloomDownsampleConfig(bloomDownPass_.renderPass())),
      bloomUpsamplePipeline_(device_, Pipeline::bloomUpsampleConfig(bloomUpPass_.renderPass())),
      frame_(device_, swapchain_),
      resources_(device_, pipelineOpaque_),
      shadowMapSize_(shadowMapExtent),
      environmentPath_(std::move(environmentPath))
{
    swapchain_.createDepthResources(device_);
    offscreenColourHandle_ = resources_.createOffscreenColourTarget(swapchain_.extent());
    forwardPass_.createForwardFramebuffer(device_,
                                          resources_.vulkanImageView(offscreenColourHandle_),
                                          swapchain_.depthView(), swapchain_.extent());
    postProcessPass_.createPostProcessFramebuffers(device_, swapchain_);
    buildBloomResources();
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
    postProcessDescSets_ = resources_.createPostProcessDescriptors(
        postProcessPipeline_.descriptorSetLayout(), offscreenColourHandle_, bloomChainHandle_);

    lightUbo_ = resources_.createMappedUniformBuffers(sizeof(LightUBO));
    resources_.lightBuffers(lightUbo_.buffers);

    shadowMapHandle_ = resources_.createShadowMap(shadowMapSize_, shadowMapLayers_);
    shadowColourHandle_ = resources_.createShadowColourAttachment(shadowMapSize_);
    std::vector<vk::ImageView> cascadeDepthViews;
    cascadeDepthViews.reserve(shadowMapLayers_);
    for (uint32_t layer = 0; layer < shadowMapLayers_; ++layer)
    {
        cascadeDepthViews.push_back(resources_.vulkanShadowMapLayerView(shadowMapHandle_, layer));
    }
    shadowPass_.createShadowFramebuffer(device_, resources_.vulkanImageView(shadowColourHandle_),
                                        cascadeDepthViews, shadowMapSize_);
    resources_.shadowDescriptorSetLayout(shadowPipeline_.descriptorSetLayout());
    resources_.shadowMap(shadowMapHandle_);
    createSkyboxEnvironment();
    createIrradianceEnvironment();
    createPrefilteredEnvironment();
    createBrdfLut();
    resources_.irradianceMap(irradianceCubemapHandle_);
    resources_.prefilteredMap(prefilteredCubemapHandle_);
    resources_.brdfLut(brdfLutHandle_);
    imagesInFlight_.assign(swapchain_.images().size(), vk::Fence{});
}

void Renderer::createSkyboxEnvironment()
{
    createSkyboxEnvironmentGpu();
}

void Renderer::createSkyboxEnvironmentGpu()
{
    Image hdr = loadEnvironmentImage(environmentPath_, "skybox cubemap creation");

    SamplerSettings sourceSampler{};
    sourceSampler.wrapS = WrapMode::Repeat;
    sourceSampler.wrapT = WrapMode::ClampToEdge;

    SamplerSettings cubemapSampler{};
    cubemapSampler.wrapS = WrapMode::ClampToEdge;
    cubemapSampler.wrapT = WrapMode::ClampToEdge;

    TextureHandle equirectHandle = resources_.createTexture(hdr, sourceSampler);

    try
    {
        skyboxCubemapHandle_ =
            resources_.createRenderTargetCubemap(skyboxCubemapExtent, skyboxCubemapMipLevels,
                                                 vk::Format::eR32G32B32A32Sfloat, cubemapSampler);

        RenderPass environmentPass = RenderPass::createOffscreenColour(
            device_, resources_.textureFormat(skyboxCubemapHandle_));
        Pipeline environmentPipeline(
            device_, Pipeline::environmentConvertConfig(environmentPass.renderPass()));

        std::array<vk::ImageView, 6> faceViews = {
            resources_.vulkanCubemapFaceView(skyboxCubemapHandle_, 0),
            resources_.vulkanCubemapFaceView(skyboxCubemapHandle_, 1),
            resources_.vulkanCubemapFaceView(skyboxCubemapHandle_, 2),
            resources_.vulkanCubemapFaceView(skyboxCubemapHandle_, 3),
            resources_.vulkanCubemapFaceView(skyboxCubemapHandle_, 4),
            resources_.vulkanCubemapFaceView(skyboxCubemapHandle_, 5),
        };
        environmentPass.createColourFramebuffers(device_, faceViews, skyboxCubemapExtent);

        auto captureSets = resources_.createSingleImageSamplerDescriptors(
            environmentPipeline.descriptorSetLayout(), equirectHandle);

        vk::CommandPoolCreateInfo poolCi{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device_.graphicsFamily(),
        };
        vk::raii::CommandPool commandPool(device_.device(), poolCi);
        vk::CommandBufferAllocateInfo cmdAi{
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        auto cmds = device_.device().allocateCommandBuffers(cmdAi);
        auto& cmd = cmds[0];
        cmd.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });

        vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(skyboxCubemapExtent),
            .height = static_cast<float>(skyboxCubemapExtent),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vk::Rect2D renderArea{
            .offset = vk::Offset2D{.x = 0, .y = 0},
            .extent = vk::Extent2D{.width = skyboxCubemapExtent, .height = skyboxCubemapExtent},
        };
        vk::ClearValue clearColour{
            .color = vk::ClearColorValue{.float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        };

        for (uint32_t face = 0; face < 6; ++face)
        {
            EnvironmentCaptureUBO capture{};
            capture.faceIndex = static_cast<int>(face);
            capture.faceExtent = static_cast<int>(skyboxCubemapExtent);

            vk::RenderPassBeginInfo beginInfo{
                .renderPass = environmentPass.renderPass(),
                .framebuffer = environmentPass.framebuffer(face),
                .renderArea = renderArea,
                .clearValueCount = 1,
                .pClearValues = &clearColour,
            };
            cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
            cmd.setViewport(0, viewport);
            cmd.setScissor(0, renderArea);
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, environmentPipeline.pipeline());
            vk::DescriptorSet ds = resources_.vulkanDescriptorSet(captureSets[0]);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   environmentPipeline.pipelineLayout(), 0, ds, {});
            cmd.pushConstants<EnvironmentCaptureUBO>(environmentPipeline.pipelineLayout(),
                                                     vk::ShaderStageFlagBits::eFragment, 0,
                                                     capture);
            cmd.draw(3, 1, 0, 0);
            cmd.endRenderPass();
        }

        // Generate mips 1..N-1 on the skybox cubemap via blit chain so the
        // prefilter pass can do Filament mip-weighted importance sampling.
        // The render pass left mip 0 (all 6 layers) in eShaderReadOnlyOptimal;
        // mips 1..N-1 are still eUndefined.
        const uint32_t skyboxMips = skyboxCubemapMipLevels;
        if (skyboxMips > 1)
        {
            vk::Image skyboxImage = resources_.vulkanImage(skyboxCubemapHandle_);

            auto barrier = [&](uint32_t baseMip, uint32_t mipCount, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout, vk::AccessFlags srcAccess,
                               vk::AccessFlags dstAccess, vk::PipelineStageFlags srcStage,
                               vk::PipelineStageFlags dstStage)
            {
                vk::ImageMemoryBarrier b{
                    .srcAccessMask = srcAccess,
                    .dstAccessMask = dstAccess,
                    .oldLayout = oldLayout,
                    .newLayout = newLayout,
                    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                    .image = skyboxImage,
                    .subresourceRange =
                        vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                  .baseMipLevel = baseMip,
                                                  .levelCount = mipCount,
                                                  .baseArrayLayer = 0,
                                                  .layerCount = 6},
                };
                cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {}, b);
            };

            // Mip 0: shader-read-only → transfer-src
            barrier(0, 1, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eShaderRead,
                    vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eFragmentShader,
                    vk::PipelineStageFlagBits::eTransfer);

            int32_t srcExtent = static_cast<int32_t>(skyboxCubemapExtent);
            for (uint32_t mip = 1; mip < skyboxMips; ++mip)
            {
                int32_t dstExtent = std::max(1, srcExtent / 2);

                // Destination mip: undefined → transfer-dst
                barrier(mip, 1, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                        {}, vk::AccessFlagBits::eTransferWrite,
                        vk::PipelineStageFlagBits::eTopOfPipe,
                        vk::PipelineStageFlagBits::eTransfer);

                vk::ImageBlit blit{
                    .srcSubresource =
                        vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                   .mipLevel = mip - 1,
                                                   .baseArrayLayer = 0,
                                                   .layerCount = 6},
                    .srcOffsets = {{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                    vk::Offset3D{.x = srcExtent, .y = srcExtent, .z = 1}}},
                    .dstSubresource =
                        vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                   .mipLevel = mip,
                                                   .baseArrayLayer = 0,
                                                   .layerCount = 6},
                    .dstOffsets = {{vk::Offset3D{.x = 0, .y = 0, .z = 0},
                                    vk::Offset3D{.x = dstExtent, .y = dstExtent, .z = 1}}},
                };
                cmd.blitImage(skyboxImage, vk::ImageLayout::eTransferSrcOptimal, skyboxImage,
                              vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

                // Previous mip → shader-read-only (done as a blit source).
                barrier(mip - 1, 1, vk::ImageLayout::eTransferSrcOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eTransferRead,
                        vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eFragmentShader);

                // This mip becomes the source for the next iteration.
                barrier(mip, 1, vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eTransferWrite,
                        vk::AccessFlagBits::eTransferRead, vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eTransfer);

                srcExtent = dstExtent;
            }

            // Final mip: transfer-src → shader-read-only.
            barrier(skyboxMips - 1, 1, vk::ImageLayout::eTransferSrcOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eTransferRead,
                    vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eFragmentShader);
        }

        cmd.end();

        vk::CommandBuffer rawCmd = *cmd;
        vk::SubmitInfo submitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCmd,
        };
        device_.graphicsQueue().submit(submitInfo);
        device_.graphicsQueue().waitIdle();
    }
    catch (const std::exception& e)
    {
        resources_.releaseTexture(equirectHandle);
        throw std::runtime_error(std::string("Environment bootstrap failed during skybox cubemap "
                                             "capture: ") +
                                 e.what());
    }

    resources_.releaseTexture(equirectHandle);
    skyboxDescSets_ = resources_.createSkyboxDescriptors(
        skyboxPipeline_.descriptorSetLayout(), skyboxUbo_, sizeof(SkyboxUBO), skyboxCubemapHandle_,
        lightUbo_, sizeof(LightUBO));
}

void Renderer::createIrradianceEnvironment()
{
    try
    {
        SamplerSettings sampler{};
        sampler.wrapS = WrapMode::ClampToEdge;
        sampler.wrapT = WrapMode::ClampToEdge;

        irradianceCubemapHandle_ = resources_.createRenderTargetCubemap(
            irradianceCubemapExtent, 1, vk::Format::eR32G32B32A32Sfloat, sampler);

        RenderPass irradiancePass = RenderPass::createOffscreenColour(
            device_, resources_.textureFormat(irradianceCubemapHandle_));
        Pipeline irradiancePipeline(
            device_, Pipeline::irradianceConvolutionConfig(irradiancePass.renderPass()));

        std::array<vk::ImageView, 6> faceViews = {
            resources_.vulkanCubemapFaceView(irradianceCubemapHandle_, 0),
            resources_.vulkanCubemapFaceView(irradianceCubemapHandle_, 1),
            resources_.vulkanCubemapFaceView(irradianceCubemapHandle_, 2),
            resources_.vulkanCubemapFaceView(irradianceCubemapHandle_, 3),
            resources_.vulkanCubemapFaceView(irradianceCubemapHandle_, 4),
            resources_.vulkanCubemapFaceView(irradianceCubemapHandle_, 5),
        };
        irradiancePass.createColourFramebuffers(device_, faceViews, irradianceCubemapExtent);

        auto captureSets = resources_.createSingleImageSamplerDescriptors(
            irradiancePipeline.descriptorSetLayout(), skyboxCubemapHandle_);

        vk::CommandPoolCreateInfo poolCi{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device_.graphicsFamily(),
        };
        vk::raii::CommandPool commandPool(device_.device(), poolCi);
        vk::CommandBufferAllocateInfo cmdAi{
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        auto cmds = device_.device().allocateCommandBuffers(cmdAi);
        auto& cmd = cmds[0];
        cmd.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });

        vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(irradianceCubemapExtent),
            .height = static_cast<float>(irradianceCubemapExtent),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vk::Rect2D renderArea{
            .offset = vk::Offset2D{.x = 0, .y = 0},
            .extent =
                vk::Extent2D{.width = irradianceCubemapExtent, .height = irradianceCubemapExtent},
        };
        vk::ClearValue clearColour{
            .color = vk::ClearColorValue{.float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        };

        for (uint32_t face = 0; face < 6; ++face)
        {
            EnvironmentCaptureUBO capture{};
            capture.faceIndex = static_cast<int>(face);
            capture.faceExtent = static_cast<int>(irradianceCubemapExtent);

            vk::RenderPassBeginInfo beginInfo{
                .renderPass = irradiancePass.renderPass(),
                .framebuffer = irradiancePass.framebuffer(face),
                .renderArea = renderArea,
                .clearValueCount = 1,
                .pClearValues = &clearColour,
            };
            cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
            cmd.setViewport(0, viewport);
            cmd.setScissor(0, renderArea);
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, irradiancePipeline.pipeline());
            vk::DescriptorSet ds = resources_.vulkanDescriptorSet(captureSets[0]);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   irradiancePipeline.pipelineLayout(), 0, ds, {});
            cmd.pushConstants<EnvironmentCaptureUBO>(irradiancePipeline.pipelineLayout(),
                                                     vk::ShaderStageFlagBits::eFragment, 0,
                                                     capture);
            cmd.draw(3, 1, 0, 0);
            cmd.endRenderPass();
        }

        cmd.end();

        vk::CommandBuffer rawCmd = *cmd;
        vk::SubmitInfo submitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCmd,
        };
        device_.graphicsQueue().submit(submitInfo);
        device_.graphicsQueue().waitIdle();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Environment bootstrap failed during irradiance "
                                             "cubemap capture: ") +
                                 e.what());
    }
}

void Renderer::createPrefilteredEnvironment()
{
    try
    {
        SamplerSettings sampler{};
        sampler.wrapS = WrapMode::ClampToEdge;
        sampler.wrapT = WrapMode::ClampToEdge;

        prefilteredCubemapHandle_ = resources_.createRenderTargetCubemap(
            prefilteredCubemapExtent, prefilteredCubemapMipLevels, vk::Format::eR32G32B32A32Sfloat,
            sampler);

        RenderPass prefilterPassTemplate = RenderPass::createOffscreenColour(
            device_, resources_.textureFormat(prefilteredCubemapHandle_));
        Pipeline prefilterPipeline(
            device_, Pipeline::prefilterEnvironmentConfig(prefilterPassTemplate.renderPass()));
        auto captureSets = resources_.createSingleImageSamplerDescriptors(
            prefilterPipeline.descriptorSetLayout(), skyboxCubemapHandle_);

        std::vector<RenderPass> prefilterPasses;
        prefilterPasses.reserve(prefilteredCubemapMipLevels);

        uint32_t mipExtent = prefilteredCubemapExtent;
        for (uint32_t level = 0; level < prefilteredCubemapMipLevels; ++level)
        {
            RenderPass mipPass = RenderPass::createOffscreenColour(
                device_, resources_.textureFormat(prefilteredCubemapHandle_));
            std::array<vk::ImageView, 6> faceViews = {
                resources_.vulkanCubemapFaceView(prefilteredCubemapHandle_, 0, level),
                resources_.vulkanCubemapFaceView(prefilteredCubemapHandle_, 1, level),
                resources_.vulkanCubemapFaceView(prefilteredCubemapHandle_, 2, level),
                resources_.vulkanCubemapFaceView(prefilteredCubemapHandle_, 3, level),
                resources_.vulkanCubemapFaceView(prefilteredCubemapHandle_, 4, level),
                resources_.vulkanCubemapFaceView(prefilteredCubemapHandle_, 5, level),
            };
            mipPass.createColourFramebuffers(device_, faceViews, mipExtent);
            prefilterPasses.push_back(std::move(mipPass));
            mipExtent = std::max(1u, mipExtent / 2);
        }

        vk::CommandPoolCreateInfo poolCi{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device_.graphicsFamily(),
        };
        vk::raii::CommandPool commandPool(device_.device(), poolCi);
        vk::CommandBufferAllocateInfo cmdAi{
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        auto cmds = device_.device().allocateCommandBuffers(cmdAi);
        auto& cmd = cmds[0];
        cmd.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });

        vk::ClearValue clearColour{
            .color = vk::ClearColorValue{.float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        };

        mipExtent = prefilteredCubemapExtent;
        for (uint32_t level = 0; level < prefilteredCubemapMipLevels; ++level)
        {
            float roughness = prefilteredCubemapMipLevels > 1
                                  ? static_cast<float>(level) /
                                        static_cast<float>(prefilteredCubemapMipLevels - 1)
                                  : 0.0f;

            vk::Viewport viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(mipExtent),
                .height = static_cast<float>(mipExtent),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            vk::Rect2D renderArea{
                .offset = vk::Offset2D{.x = 0, .y = 0},
                .extent = vk::Extent2D{.width = mipExtent, .height = mipExtent},
            };

            for (uint32_t face = 0; face < 6; ++face)
            {
                EnvironmentPrefilterPushConstants capture{};
                capture.faceIndex = static_cast<int>(face);
                capture.faceExtent = static_cast<int>(mipExtent);
                capture.roughness = roughness;
                capture.sourceFaceExtent = static_cast<int>(skyboxCubemapExtent);
                capture.sourceMaxMip = static_cast<float>(skyboxCubemapMipLevels - 1);

                vk::RenderPassBeginInfo beginInfo{
                    .renderPass = prefilterPasses[level].renderPass(),
                    .framebuffer = prefilterPasses[level].framebuffer(face),
                    .renderArea = renderArea,
                    .clearValueCount = 1,
                    .pClearValues = &clearColour,
                };
                cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
                cmd.setViewport(0, viewport);
                cmd.setScissor(0, renderArea);
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, prefilterPipeline.pipeline());
                vk::DescriptorSet ds = resources_.vulkanDescriptorSet(captureSets[0]);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       prefilterPipeline.pipelineLayout(), 0, ds, {});
                cmd.pushConstants<EnvironmentPrefilterPushConstants>(
                    prefilterPipeline.pipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0,
                    capture);
                cmd.draw(3, 1, 0, 0);
                cmd.endRenderPass();
            }

            mipExtent = std::max(1u, mipExtent / 2);
        }

        cmd.end();

        vk::CommandBuffer rawCmd = *cmd;
        vk::SubmitInfo submitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCmd,
        };
        device_.graphicsQueue().submit(submitInfo);
        device_.graphicsQueue().waitIdle();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Environment bootstrap failed during prefiltered "
                                             "cubemap capture: ") +
                                 e.what());
    }
}

void Renderer::createBrdfLut()
{
    try
    {
        brdfLutHandle_ = resources_.createOffscreenColourTarget({brdfLutExtent, brdfLutExtent});

        RenderPass brdfPass =
            RenderPass::createOffscreenColour(device_, resources_.textureFormat(brdfLutHandle_));
        Pipeline brdfPipeline(device_, Pipeline::brdfIntegrationConfig(brdfPass.renderPass()));

        std::array<vk::ImageView, 1> views = {resources_.vulkanImageView(brdfLutHandle_)};
        brdfPass.createColourFramebuffers(device_, views, brdfLutExtent);

        vk::CommandPoolCreateInfo poolCi{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = device_.graphicsFamily(),
        };
        vk::raii::CommandPool commandPool(device_.device(), poolCi);
        vk::CommandBufferAllocateInfo cmdAi{
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        auto cmds = device_.device().allocateCommandBuffers(cmdAi);
        auto& cmd = cmds[0];
        cmd.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });

        vk::Viewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(brdfLutExtent),
            .height = static_cast<float>(brdfLutExtent),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vk::Rect2D renderArea{
            .offset = vk::Offset2D{.x = 0, .y = 0},
            .extent = vk::Extent2D{.width = brdfLutExtent, .height = brdfLutExtent},
        };
        vk::ClearValue clearColour{
            .color = vk::ClearColorValue{.float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}},
        };

        vk::RenderPassBeginInfo beginInfo{
            .renderPass = brdfPass.renderPass(),
            .framebuffer = brdfPass.framebuffer(0),
            .renderArea = renderArea,
            .clearValueCount = 1,
            .pClearValues = &clearColour,
        };
        cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
        cmd.setViewport(0, viewport);
        cmd.setScissor(0, renderArea);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, brdfPipeline.pipeline());
        cmd.draw(3, 1, 0, 0);
        cmd.endRenderPass();
        cmd.end();

        vk::CommandBuffer rawCmd = *cmd;
        vk::SubmitInfo submitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &rawCmd,
        };
        device_.graphicsQueue().submit(submitInfo);
        device_.graphicsQueue().waitIdle();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Environment bootstrap failed during BRDF LUT "
                                             "capture: ") +
                                 e.what());
    }
}

void Renderer::updateLightData(Vec3 cameraPosition, Vec3 cameraTarget, float aspect)
{
    const Vec3 lightDir = Vec3::normalise({-1.0f, 1.0f, -1.0f});

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
    lightData.direction[0] = lightDir.x();
    lightData.direction[1] = lightDir.y();
    lightData.direction[2] = lightDir.z();
    lightData.direction[3] = 0.0f;
    lightData.colour[0] = 1.0f;
    lightData.colour[1] = 1.0f;
    lightData.colour[2] = 1.0f;
    lightData.colour[3] = directionalLightIntensity;
    for (uint32_t i = 0; i < shadowCascadeCount; ++i)
    {
        lightData.cascadeViewProj[i] = cascadeViewProj[i];
        lightData.cascadeSplits[i] = splits[i];
    }
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
    std::array<vk::ClearValue, 2> shadowClears = {
        vk::ClearValue{.color = vk::ClearColorValue{.float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}}},
        vk::ClearValue{.depthStencil = vk::ClearDepthStencilValue{.depth = 1.0f, .stencil = 0}},
    };

    vk::Viewport shadowVp{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(shadowMapSize_),
        .height = static_cast<float>(shadowMapSize_),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vk::Rect2D shadowScissor{
        .offset = vk::Offset2D{.x = 0, .y = 0},
        .extent = vk::Extent2D{.width = shadowMapSize_, .height = shadowMapSize_},
    };

    // Render every caster into every cascade. The per-cascade lightViewProj
    // already lives in ShadowUBO (written by Object::render from FrameInfo);
    // the push constant picks which entry the shadow vertex shader uses.
    const vk::PipelineLayout shadowPipelineLayout =
        resources_.vulkanPipelineLayout(shadowPipelineHandle_);
    for (uint32_t cascade = 0; cascade < shadowMapLayers_; ++cascade)
    {
        vk::RenderPassBeginInfo shadowBegin{
            .renderPass = shadowPass_.renderPass(),
            .framebuffer = shadowPass_.framebuffer(cascade),
            .renderArea = shadowScissor,
            .clearValueCount = static_cast<uint32_t>(shadowClears.size()),
            .pClearValues = shadowClears.data(),
        };
        cmd.beginRenderPass(shadowBegin, vk::SubpassContents::eInline);
        cmd.setViewport(0, shadowVp);
        cmd.setScissor(0, shadowScissor);

        ShadowPushConstants pc{};
        pc.cascadeIndex = static_cast<int>(cascade);
        cmd.pushConstants<ShadowPushConstants>(shadowPipelineLayout,
                                               vk::ShaderStageFlagBits::eVertex, 0, pc);

        auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
        recordDrawBucket(cmd, shadowDraws, lastBoundPipeline);

        cmd.endRenderPass();
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

void Renderer::buildBloomResources()
{
    auto extent = swapchain_.extent();
    uint32_t bloomWidth = std::max(1u, extent.width / 2);
    uint32_t bloomHeight = std::max(1u, extent.height / 2);

    bloomChainHandle_ = resources_.createBloomChain(bloomWidth, bloomHeight, bloomMipCount);

    // Downsample writes every mip (0..N-1). Per-mip framebuffers + extents.
    std::vector<vk::ImageView> downViews;
    std::vector<vk::Extent2D> downExtents;
    downViews.reserve(bloomMipCount);
    downExtents.reserve(bloomMipCount);
    for (uint32_t m = 0; m < bloomMipCount; ++m)
    {
        downViews.push_back(resources_.vulkanBloomMipView(bloomChainHandle_, m));
        downExtents.push_back({std::max(1u, bloomWidth >> m), std::max(1u, bloomHeight >> m)});
    }
    bloomDownPass_.createColourFramebuffersPerMip(device_, downViews, downExtents);

    // Upsample writes mips 0..N-2 only (mip N-1 stays as the source for the
    // first upsample step). One framebuffer per writable mip.
    std::vector<vk::ImageView> upViews;
    std::vector<vk::Extent2D> upExtents;
    upViews.reserve(bloomMipCount - 1);
    upExtents.reserve(bloomMipCount - 1);
    for (uint32_t m = 0; m < bloomMipCount - 1; ++m)
    {
        upViews.push_back(resources_.vulkanBloomMipView(bloomChainHandle_, m));
        upExtents.push_back({std::max(1u, bloomWidth >> m), std::max(1u, bloomHeight >> m)});
    }
    bloomUpPass_.createColourFramebuffersPerMip(device_, upViews, upExtents);

    // Descriptor sets. Downsample[0] reads the HDR target; the rest read the
    // previous bloom mip. Upsample[m] reads bloom mip m+1, writes mip m.
    bloomDownDescSets_.clear();
    bloomDownDescSets_.reserve(bloomMipCount);
    bloomDownDescSets_.push_back(
        resources_.createImageViewDescriptor(bloomDownsamplePipeline_.descriptorSetLayout(),
                                             resources_.vulkanImageView(offscreenColourHandle_),
                                             resources_.vulkanSampler(offscreenColourHandle_)));
    for (uint32_t m = 0; m < bloomMipCount - 1; ++m)
    {
        bloomDownDescSets_.push_back(resources_.createImageViewDescriptor(
            bloomDownsamplePipeline_.descriptorSetLayout(),
            resources_.vulkanBloomMipView(bloomChainHandle_, m),
            resources_.vulkanSampler(bloomChainHandle_)));
    }

    bloomUpDescSets_.clear();
    bloomUpDescSets_.reserve(bloomMipCount - 1);
    for (uint32_t m = 0; m < bloomMipCount - 1; ++m)
    {
        bloomUpDescSets_.push_back(resources_.createImageViewDescriptor(
            bloomUpsamplePipeline_.descriptorSetLayout(),
            resources_.vulkanBloomMipView(bloomChainHandle_, m + 1),
            resources_.vulkanSampler(bloomChainHandle_)));
    }
}

void Renderer::recordBloomPasses(vk::CommandBuffer cmd)
{
    auto extent = swapchain_.extent();
    uint32_t bloomWidth = std::max(1u, extent.width / 2);
    uint32_t bloomHeight = std::max(1u, extent.height / 2);

    const vk::PipelineLayout downLayout = bloomDownsamplePipeline_.pipelineLayout();
    const vk::PipelineLayout upLayout = bloomUpsamplePipeline_.pipelineLayout();

    // Downsample chain: HDR → mip 0 → mip 1 → ... → mip N-1.
    for (uint32_t m = 0; m < bloomMipCount; ++m)
    {
        uint32_t dstW = std::max(1u, bloomWidth >> m);
        uint32_t dstH = std::max(1u, bloomHeight >> m);
        // Source dimensions: mip 0 reads the full-res HDR target; subsequent
        // passes read the previous (finer) mip of the bloom chain.
        uint32_t srcW = (m == 0) ? extent.width : std::max(1u, bloomWidth >> (m - 1));
        uint32_t srcH = (m == 0) ? extent.height : std::max(1u, bloomHeight >> (m - 1));

        vk::Rect2D renderArea{
            .offset = vk::Offset2D{.x = 0, .y = 0},
            .extent = vk::Extent2D{.width = dstW, .height = dstH},
        };
        vk::RenderPassBeginInfo begin{
            .renderPass = bloomDownPass_.renderPass(),
            .framebuffer = bloomDownPass_.framebuffer(m),
            .renderArea = renderArea,
        };
        cmd.beginRenderPass(begin, vk::SubpassContents::eInline);
        cmd.setViewport(0, vk::Viewport{.x = 0.0f,
                                        .y = 0.0f,
                                        .width = static_cast<float>(dstW),
                                        .height = static_cast<float>(dstH),
                                        .minDepth = 0.0f,
                                        .maxDepth = 1.0f});
        cmd.setScissor(0, renderArea);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, bloomDownsamplePipeline_.pipeline());
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, downLayout, 0,
                               resources_.vulkanDescriptorSet(bloomDownDescSets_[m]), {});
        BloomPushConstants pc{};
        pc.invInputResolution[0] = 1.0f / static_cast<float>(srcW);
        pc.invInputResolution[1] = 1.0f / static_cast<float>(srcH);
        pc.isFirstPass = (m == 0) ? 1 : 0;
        cmd.pushConstants<BloomPushConstants>(downLayout, vk::ShaderStageFlagBits::eFragment, 0,
                                              pc);
        cmd.draw(3, 1, 0, 0);
        cmd.endRenderPass();
    }

    // Upsample chain: mip N-1 → mip N-2 (additive) → ... → mip 0.
    for (int m = static_cast<int>(bloomMipCount) - 2; m >= 0; --m)
    {
        uint32_t dstW = std::max(1u, bloomWidth >> m);
        uint32_t dstH = std::max(1u, bloomHeight >> m);
        uint32_t srcW = std::max(1u, bloomWidth >> (m + 1));
        uint32_t srcH = std::max(1u, bloomHeight >> (m + 1));

        vk::Rect2D renderArea{
            .offset = vk::Offset2D{.x = 0, .y = 0},
            .extent = vk::Extent2D{.width = dstW, .height = dstH},
        };
        vk::RenderPassBeginInfo begin{
            .renderPass = bloomUpPass_.renderPass(),
            .framebuffer = bloomUpPass_.framebuffer(static_cast<uint32_t>(m)),
            .renderArea = renderArea,
        };
        cmd.beginRenderPass(begin, vk::SubpassContents::eInline);
        cmd.setViewport(0, vk::Viewport{.x = 0.0f,
                                        .y = 0.0f,
                                        .width = static_cast<float>(dstW),
                                        .height = static_cast<float>(dstH),
                                        .minDepth = 0.0f,
                                        .maxDepth = 1.0f});
        cmd.setScissor(0, renderArea);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, bloomUpsamplePipeline_.pipeline());
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, upLayout, 0,
                               resources_.vulkanDescriptorSet(bloomUpDescSets_[m]), {});
        BloomPushConstants pc{};
        pc.invInputResolution[0] = 1.0f / static_cast<float>(srcW);
        pc.invInputResolution[1] = 1.0f / static_cast<float>(srcH);
        pc.isFirstPass = 0;
        cmd.pushConstants<BloomPushConstants>(upLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);
        cmd.draw(3, 1, 0, 0);
        cmd.endRenderPass();
    }
}

void Renderer::transitionOffscreenForSampling(vk::CommandBuffer cmd)
{
    vk::ImageMemoryBarrier offscreenReadyForSampling{
        .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = resources_.vulkanImage(offscreenColourHandle_),
        .subresourceRange = vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                      .baseMipLevel = 0,
                                                      .levelCount = 1,
                                                      .baseArrayLayer = 0,
                                                      .layerCount = 1},
    };
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                        offscreenReadyForSampling);
}

void Renderer::recordPostProcessPass(vk::CommandBuffer cmd, uint32_t imageIndex)
{
    auto extent = swapchain_.extent();
    vk::Rect2D renderArea{
        .offset = vk::Offset2D{.x = 0, .y = 0},
        .extent = extent,
    };
    vk::RenderPassBeginInfo ppBegin{
        .renderPass = postProcessPass_.renderPass(),
        .framebuffer = postProcessPass_.framebuffer(imageIndex),
        .renderArea = renderArea,
    };
    cmd.beginRenderPass(ppBegin, vk::SubpassContents::eInline);

    vk::Viewport vp{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    cmd.setViewport(0, vp);
    cmd.setScissor(0, renderArea);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                     resources_.vulkanPipeline(postProcessPipelineHandle_));

    vk::DescriptorSet ppSet = resources_.vulkanDescriptorSet(postProcessDescSets_[currentFrame_]);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                           resources_.vulkanPipelineLayout(postProcessPipelineHandle_), 0, ppSet,
                           {});
    PostProcessPushConstants ppc{};
    ppc.bloomStrength = bloomStrength;
    cmd.pushConstants<PostProcessPushConstants>(
        resources_.vulkanPipelineLayout(postProcessPipelineHandle_),
        vk::ShaderStageFlagBits::eFragment, 0, ppc);
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

    const auto extent = swapchain_.extent();
    const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    updateLightData(cameraPosition, cameraTarget, aspect);

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
                      cascadeViewProjs_};
    scene.render(ctx);

    DrawBuckets buckets = buildDrawBuckets(drawCommands);
    recordShadowPass(cmd, buckets.shadow);
    recordForwardPass(cmd, buckets);
    transitionOffscreenForSampling(cmd);
    recordBloomPasses(cmd);
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
    // Bloom chain follows swapchain — drop the old one, build at the new size.
    resources_.releaseTexture(bloomChainHandle_);
    buildBloomResources();
    resources_.updatePostProcessDescriptors(postProcessDescSets_, offscreenColourHandle_,
                                            bloomChainHandle_);
    frame_.createRenderFinishedSemaphores(swapchain_.images().size());
    imagesInFlight_.assign(swapchain_.images().size(), vk::Fence{});
}

} // namespace fire_engine
