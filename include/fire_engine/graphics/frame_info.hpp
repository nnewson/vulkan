#pragma once

#include <array>
#include <cstdint>

#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

// Pipeline handles covering the three forward alpha variants. Object::render
// picks one per geometry from the material's alphaMode/doubleSided flags. The
// renderer later buckets blend draws so they can be sorted back-to-front.
struct AlphaPipelines
{
    PipelineHandle opaque{NullPipeline};
    PipelineHandle opaqueDoubleSided{NullPipeline};
    PipelineHandle blend{NullPipeline};
};

struct FrameInfo
{
    uint32_t currentFrame{0};
    uint32_t viewportWidth{0};
    uint32_t viewportHeight{0};
    Vec3 cameraPosition;
    Vec3 cameraTarget;
    AlphaPipelines pipelines{};
    PipelineHandle shadowPipeline{NullPipeline};
    // Per-cascade light-space view-projection matrices. Object::render copies
    // all four into the per-draw ShadowUBO so the shadow vertex shader can
    // select by push-constant cascade index.
    std::array<Mat4, 4> cascadeViewProjs{};
};

} // namespace fire_engine
