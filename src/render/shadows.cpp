#include <fire_engine/render/shadows.hpp>

#include <array>
#include <limits>

#include <fire_engine/render/device.hpp>
#include <fire_engine/render/ubo.hpp>

namespace fire_engine
{

namespace
{

void recordShadowDrawBucket(vk::CommandBuffer cmd, const std::vector<DrawCommand>& shadowDraws,
                            const Resources& resources)
{
    auto lastBoundPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};
    for (const auto& dc : shadowDraws)
    {
        if (dc.pipeline != lastBoundPipeline)
        {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             resources.vulkanPipeline(dc.pipeline));
            lastBoundPipeline = dc.pipeline;
        }
        if (dc.vertexBuffer != NullBuffer)
        {
            cmd.bindVertexBuffers(0, resources.vulkanBuffer(dc.vertexBuffer), {vk::DeviceSize{0}});
        }

        vk::IndexType indexType =
            dc.indexType == DrawIndexType::UInt32 ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
        cmd.bindIndexBuffer(resources.vulkanBuffer(dc.indexBuffer), 0, indexType);

        vk::DescriptorSet ds = resources.vulkanDescriptorSet(dc.descriptorSet);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               resources.vulkanPipelineLayout(dc.pipeline), 0, ds, {});
        cmd.drawIndexed(dc.indexCount, 1, 0, 0, 0);
    }
}

} // namespace

Shadows::Shadows(const Device& device, Resources& resources)
    : device_{&device},
      resources_{&resources},
      shadowPass_(RenderPass::createShadow(device)),
      shadowPipeline_(device, Pipeline::shadowConfig(shadowPass_.renderPass()))
{
    shadowPipelineHandle_ =
        resources_->registerPipeline(shadowPipeline_.pipeline(), shadowPipeline_.pipelineLayout());
    shadowMapHandle_ = resources_->createShadowMap(shadowMapExtent, shadowCascadeCount);
    shadowColourHandle_ = resources_->createShadowColourAttachment(shadowMapExtent);

    std::vector<vk::ImageView> cascadeDepthViews;
    cascadeDepthViews.reserve(shadowCascadeCount);
    for (uint32_t layer = 0; layer < shadowCascadeCount; ++layer)
    {
        cascadeDepthViews.push_back(resources_->vulkanShadowMapLayerView(shadowMapHandle_, layer));
    }

    shadowPass_.createShadowFramebuffer(*device_, resources_->vulkanImageView(shadowColourHandle_),
                                        cascadeDepthViews, shadowMapExtent);
    resources_->descriptors().shadowDescriptorSetLayout(shadowPipeline_.descriptorSetLayout());
    resources_->shadowMap(shadowMapHandle_);
}

void Shadows::recordPass(vk::CommandBuffer cmd, const std::vector<DrawCommand>& shadowDraws) const
{
    std::array<vk::ClearValue, 2> shadowClears = {
        vk::ClearValue{.color = vk::ClearColorValue{.float32 = {{0.0f, 0.0f, 0.0f, 1.0f}}}},
        vk::ClearValue{.depthStencil = vk::ClearDepthStencilValue{.depth = 1.0f, .stencil = 0}},
    };

    vk::Viewport shadowVp{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(shadowMapExtent),
        .height = static_cast<float>(shadowMapExtent),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vk::Rect2D shadowScissor{
        .offset = vk::Offset2D{.x = 0, .y = 0},
        .extent = vk::Extent2D{.width = shadowMapExtent, .height = shadowMapExtent},
    };

    const vk::PipelineLayout shadowPipelineLayout =
        resources_->vulkanPipelineLayout(shadowPipelineHandle_);
    for (uint32_t cascade = 0; cascade < shadowCascadeCount; ++cascade)
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

        recordShadowDrawBucket(cmd, shadowDraws, *resources_);

        cmd.endRenderPass();
    }
}

} // namespace fire_engine
