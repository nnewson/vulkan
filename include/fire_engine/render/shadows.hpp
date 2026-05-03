#pragma once

#include <vector>

#include <fire_engine/graphics/draw_command.hpp>
#include <fire_engine/render/constants.hpp>
#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/render_pass.hpp>
#include <fire_engine/render/resources.hpp>

namespace fire_engine
{

class Device;

class Shadows
{
public:
    Shadows(const Device& device, Resources& resources);
    ~Shadows() = default;

    Shadows(const Shadows&) = delete;
    Shadows& operator=(const Shadows&) = delete;
    Shadows(Shadows&&) noexcept = default;
    Shadows& operator=(Shadows&&) noexcept = default;

    [[nodiscard]] PipelineHandle pipelineHandle() const noexcept
    {
        return shadowPipelineHandle_;
    }

    void recordPass(vk::CommandBuffer cmd, const std::vector<DrawCommand>& shadowDraws) const;

private:
    const Device* device_{nullptr};
    Resources* resources_{nullptr};
    RenderPass shadowPass_;
    Pipeline shadowPipeline_;
    PipelineHandle shadowPipelineHandle_{NullPipeline};
    TextureHandle shadowMapHandle_{NullTexture};
    TextureHandle shadowColourHandle_{NullTexture};
};

} // namespace fire_engine
