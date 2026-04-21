#pragma once

#include <cstdint>
#include <limits>

namespace fire_engine
{

enum class BufferHandle : uint32_t
{
};

enum class TextureHandle : uint32_t
{
};

enum class DescriptorSetHandle : uint32_t
{
};

enum class PipelineHandle : uint32_t
{
};

inline constexpr auto NullBuffer = BufferHandle{std::numeric_limits<uint32_t>::max()};
inline constexpr auto NullTexture = TextureHandle{std::numeric_limits<uint32_t>::max()};
inline constexpr auto NullDescriptorSet = DescriptorSetHandle{std::numeric_limits<uint32_t>::max()};
inline constexpr auto NullPipeline = PipelineHandle{std::numeric_limits<uint32_t>::max()};

// Semantic alias for mapped GPU memory pointers (replaces raw void*)
using MappedMemory = void*;

} // namespace fire_engine
