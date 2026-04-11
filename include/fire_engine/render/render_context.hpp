#pragma once

#include <cstdint>

#include <vulkan/vulkan.hpp>

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
};

} // namespace fire_engine
