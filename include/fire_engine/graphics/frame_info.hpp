#pragma once

#include <cstdint>

#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

struct FrameInfo
{
    uint32_t currentFrame{0};
    uint32_t viewportWidth{0};
    uint32_t viewportHeight{0};
    Vec3 cameraPosition;
    Vec3 cameraTarget;
    PipelineHandle pipeline{NullPipeline};
};

} // namespace fire_engine
