#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <fire_engine/graphics/draw_command.hpp>
#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/render/constants.hpp>
#include <fire_engine/render/device.hpp>
#include <fire_engine/render/frame.hpp>
#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/render_pass.hpp>
#include <fire_engine/render/resources.hpp>
#include <fire_engine/render/swapchain.hpp>
#include <fire_engine/render/ubo.hpp>

namespace fire_engine
{

class SceneGraph;

class Renderer
{
public:
    explicit Renderer(const Window& window, std::string environmentPath = {});
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
        return pipelineOpaque_;
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
    struct DrawBuckets
    {
        std::vector<DrawCommand> shadow;
        std::vector<DrawCommand> opaque;
        std::vector<DrawCommand> blend;
    };

    void updateLightData(Vec3 cameraPosition, Vec3 cameraTarget, float aspect);
    [[nodiscard]] DrawBuckets buildDrawBuckets(const std::vector<DrawCommand>& drawCommands) const;
    void recordDrawBucket(vk::CommandBuffer cmd, const std::vector<DrawCommand>& bucket,
                          PipelineHandle& lastBoundPipeline) const;
    void recordShadowPass(vk::CommandBuffer cmd, const std::vector<DrawCommand>& shadowDraws);
    void recordForwardPass(vk::CommandBuffer cmd, const DrawBuckets& buckets);
    void transitionOffscreenForSampling(vk::CommandBuffer cmd);
    void recordBloomPasses(vk::CommandBuffer cmd);
    void buildBloomResources();
    void recordPostProcessPass(vk::CommandBuffer cmd, uint32_t imageIndex);
    void createSkyboxEnvironment();
    void createSkyboxEnvironmentGpu();
    void createIrradianceEnvironment();
    void createPrefilteredEnvironment();
    void createBrdfLut();
    void recreateSwapchain(const Window& display);
    [[nodiscard]] std::optional<uint32_t> acquireNextImage(Window& display);
    void beginRenderPass(vk::CommandBuffer cmd);
    void submitAndPresent(Window& display, vk::CommandBuffer cmd, uint32_t imageIndex);
    void recordSkybox(Vec3 cameraPosition, Vec3 cameraTarget,
                      std::vector<DrawCommand>& drawCommands);

    Device device_;
    Swapchain swapchain_;
    RenderPass forwardPass_;
    RenderPass shadowPass_;
    RenderPass postProcessPass_;
    RenderPass bloomDownPass_;
    RenderPass bloomUpPass_;
    Pipeline pipelineOpaque_;
    Pipeline pipelineOpaqueDoubleSided_;
    Pipeline pipelineBlend_;
    Pipeline skyboxPipeline_;
    Pipeline shadowPipeline_;
    Pipeline postProcessPipeline_;
    Pipeline bloomDownsamplePipeline_;
    Pipeline bloomUpsamplePipeline_;
    Frame frame_;
    Resources resources_;
    PipelineHandle forwardOpaqueHandle_{NullPipeline};
    PipelineHandle forwardOpaqueDoubleSidedHandle_{NullPipeline};
    PipelineHandle forwardBlendHandle_{NullPipeline};
    PipelineHandle skyboxPipelineHandle_{NullPipeline};
    PipelineHandle shadowPipelineHandle_{NullPipeline};
    PipelineHandle postProcessPipelineHandle_{NullPipeline};
    TextureHandle shadowMapHandle_{NullTexture};
    TextureHandle shadowColourHandle_{NullTexture};
    TextureHandle offscreenColourHandle_{NullTexture};
    TextureHandle bloomChainHandle_{NullTexture};
    // One descriptor set per bloom pass (downsample inputs + upsample inputs).
    std::vector<DescriptorSetHandle> bloomDownDescSets_{};
    std::vector<DescriptorSetHandle> bloomUpDescSets_{};
    TextureHandle skyboxCubemapHandle_{NullTexture};
    TextureHandle irradianceCubemapHandle_{NullTexture};
    TextureHandle prefilteredCubemapHandle_{NullTexture};
    TextureHandle brdfLutHandle_{NullTexture};
    Resources::MappedBufferSet skyboxUbo_;
    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> skyboxDescSets_{};
    std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> postProcessDescSets_{};
    BufferHandle skyboxIndexBuffer_{NullBuffer};
    BufferHandle postProcessIndexBuffer_{NullBuffer};
    Resources::MappedBufferSet lightUbo_;
    std::array<Mat4, 4> cascadeViewProjs_{};
    std::vector<vk::Fence> imagesInFlight_{};
    uint32_t currentFrame_{0};
    uint32_t shadowMapSize_{0};
    uint32_t shadowMapLayers_{4};
    // Flip to true and rebuild to visualise cascade regions: red→green→blue→yellow
    // for cascades 0..3, near to far. Surfaced via LightUBO.environmentParams.w.
    bool cascadeDebugTint_{false};
    std::string environmentPath_;
};

} // namespace fire_engine
