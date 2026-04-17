#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <fire_engine/graphics/gpu_handle.hpp>

using namespace fire_engine;

// ---------------------------------------------------------------------------
// Null handle constants
// ---------------------------------------------------------------------------

TEST(GpuHandle, NullBufferIsMaxUint32)
{
    EXPECT_EQ(static_cast<uint32_t>(NullBuffer), std::numeric_limits<uint32_t>::max());
}

TEST(GpuHandle, NullTextureIsMaxUint32)
{
    EXPECT_EQ(static_cast<uint32_t>(NullTexture), std::numeric_limits<uint32_t>::max());
}

TEST(GpuHandle, NullDescriptorSetIsMaxUint32)
{
    EXPECT_EQ(static_cast<uint32_t>(NullDescriptorSet), std::numeric_limits<uint32_t>::max());
}

TEST(GpuHandle, NullPipelineIsMaxUint32)
{
    EXPECT_EQ(static_cast<uint32_t>(NullPipeline), std::numeric_limits<uint32_t>::max());
}

// ---------------------------------------------------------------------------
// Constexpr verification
// ---------------------------------------------------------------------------

TEST(GpuHandle, NullBufferIsConstexpr)
{
    static_assert(NullBuffer == BufferHandle{std::numeric_limits<uint32_t>::max()});
}

TEST(GpuHandle, NullTextureIsConstexpr)
{
    static_assert(NullTexture == TextureHandle{std::numeric_limits<uint32_t>::max()});
}

TEST(GpuHandle, NullDescriptorSetIsConstexpr)
{
    static_assert(NullDescriptorSet == DescriptorSetHandle{std::numeric_limits<uint32_t>::max()});
}

TEST(GpuHandle, NullPipelineIsConstexpr)
{
    static_assert(NullPipeline == PipelineHandle{std::numeric_limits<uint32_t>::max()});
}

// ---------------------------------------------------------------------------
// Handle equality and comparison
// ---------------------------------------------------------------------------

TEST(GpuHandle, BufferHandleEqualityWithSameValue)
{
    auto a = BufferHandle{42};
    auto b = BufferHandle{42};
    EXPECT_EQ(a, b);
}

TEST(GpuHandle, BufferHandleInequalityWithDifferentValues)
{
    auto a = BufferHandle{0};
    auto b = BufferHandle{1};
    EXPECT_NE(a, b);
}

TEST(GpuHandle, TextureHandleEqualityWithSameValue)
{
    auto a = TextureHandle{7};
    auto b = TextureHandle{7};
    EXPECT_EQ(a, b);
}

TEST(GpuHandle, DescriptorSetHandleEqualityWithSameValue)
{
    auto a = DescriptorSetHandle{3};
    auto b = DescriptorSetHandle{3};
    EXPECT_EQ(a, b);
}

TEST(GpuHandle, PipelineHandleEqualityWithSameValue)
{
    auto a = PipelineHandle{9};
    auto b = PipelineHandle{9};
    EXPECT_EQ(a, b);
}

TEST(GpuHandle, PipelineHandleInequalityWithDifferentValues)
{
    auto a = PipelineHandle{0};
    auto b = PipelineHandle{1};
    EXPECT_NE(a, b);
}

// ---------------------------------------------------------------------------
// Null vs valid handles
// ---------------------------------------------------------------------------

TEST(GpuHandle, ValidBufferHandleNotEqualToNull)
{
    auto valid = BufferHandle{0};
    EXPECT_NE(valid, NullBuffer);
}

TEST(GpuHandle, ValidTextureHandleNotEqualToNull)
{
    auto valid = TextureHandle{0};
    EXPECT_NE(valid, NullTexture);
}

TEST(GpuHandle, ValidDescriptorSetHandleNotEqualToNull)
{
    auto valid = DescriptorSetHandle{0};
    EXPECT_NE(valid, NullDescriptorSet);
}

TEST(GpuHandle, ValidPipelineHandleNotEqualToNull)
{
    auto valid = PipelineHandle{0};
    EXPECT_NE(valid, NullPipeline);
}

// ---------------------------------------------------------------------------
// Type safety — handles are distinct types
// ---------------------------------------------------------------------------

TEST(GpuHandle, HandleTypesAreDistinct)
{
    EXPECT_FALSE((std::is_same_v<BufferHandle, TextureHandle>));
    EXPECT_FALSE((std::is_same_v<BufferHandle, DescriptorSetHandle>));
    EXPECT_FALSE((std::is_same_v<TextureHandle, DescriptorSetHandle>));
    EXPECT_FALSE((std::is_same_v<BufferHandle, PipelineHandle>));
    EXPECT_FALSE((std::is_same_v<TextureHandle, PipelineHandle>));
    EXPECT_FALSE((std::is_same_v<DescriptorSetHandle, PipelineHandle>));
}

// ---------------------------------------------------------------------------
// Round-trip cast
// ---------------------------------------------------------------------------

TEST(GpuHandle, BufferHandleRoundTrip)
{
    uint32_t id = 123;
    auto handle = BufferHandle{id};
    EXPECT_EQ(static_cast<uint32_t>(handle), id);
}

TEST(GpuHandle, TextureHandleRoundTrip)
{
    uint32_t id = 456;
    auto handle = TextureHandle{id};
    EXPECT_EQ(static_cast<uint32_t>(handle), id);
}

TEST(GpuHandle, DescriptorSetHandleRoundTrip)
{
    uint32_t id = 789;
    auto handle = DescriptorSetHandle{id};
    EXPECT_EQ(static_cast<uint32_t>(handle), id);
}

TEST(GpuHandle, PipelineHandleRoundTrip)
{
    uint32_t id = 1011;
    auto handle = PipelineHandle{id};
    EXPECT_EQ(static_cast<uint32_t>(handle), id);
}

// ---------------------------------------------------------------------------
// MappedMemory alias
// ---------------------------------------------------------------------------

TEST(GpuHandle, MappedMemoryIsVoidPointer)
{
    EXPECT_TRUE((std::is_same_v<MappedMemory, void*>));
}

TEST(GpuHandle, MappedMemoryDefaultInitializesNull)
{
    MappedMemory ptr{};
    EXPECT_EQ(ptr, nullptr);
}
