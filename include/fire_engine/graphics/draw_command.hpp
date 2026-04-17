#pragma once

#include <cstdint>

#include <fire_engine/graphics/gpu_handle.hpp>

namespace fire_engine
{

struct DrawCommand
{
    BufferHandle vertexBuffer{NullBuffer};
    BufferHandle indexBuffer{NullBuffer};
    uint32_t indexCount{0};
    DescriptorSetHandle descriptorSet{NullDescriptorSet};
    PipelineHandle pipeline{NullPipeline};
};

} // namespace fire_engine
