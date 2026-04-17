#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/graphics/draw_command.hpp>
#include <fire_engine/graphics/frame_info.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Device;
class Swapchain;
class Frame;
class Pipeline;

struct RenderContext
{
    const Device& device;
    const Swapchain& swapchain;
    Frame& frame;
    const Pipeline& pipeline;
    vk::CommandBuffer commandBuffer;
    uint32_t currentFrame;
    Vec3 cameraPosition;
    Vec3 cameraTarget;
    std::vector<DrawCommand>* drawCommands{nullptr};
    PipelineHandle pipelineHandle{NullPipeline};

    [[nodiscard]] FrameInfo frameInfo() const noexcept;
};

} // namespace fire_engine
