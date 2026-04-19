#include <gtest/gtest.h>

#include <fire_engine/graphics/assets.hpp>

using namespace fire_engine;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(Assets, DefaultConstructionHasZeroCounts)
{
    Assets assets;
    EXPECT_EQ(assets.textureCount(), 0);
    EXPECT_EQ(assets.materialCount(), 0);
    EXPECT_EQ(assets.geometryCount(), 0);
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------

TEST(Assets, ResizeTexturesUpdatesCounts)
{
    Assets assets;
    assets.resizeTextures(5);
    EXPECT_EQ(assets.textureCount(), 5);
}

TEST(Assets, ResizeMaterialsUpdatesCounts)
{
    Assets assets;
    assets.resizeMaterials(3);
    EXPECT_EQ(assets.materialCount(), 3);
}

TEST(Assets, ResizeGeometriesUpdatesCounts)
{
    Assets assets;
    assets.resizeGeometries(7);
    EXPECT_EQ(assets.geometryCount(), 7);
}

// ---------------------------------------------------------------------------
// Material access and modification
// ---------------------------------------------------------------------------

TEST(Assets, MaterialAccessByIndex)
{
    Assets assets;
    assets.resizeMaterials(2);
    assets.material(0).name("mat_a");
    assets.material(1).name("mat_b");

    EXPECT_EQ(assets.material(0).name(), "mat_a");
    EXPECT_EQ(assets.material(1).name(), "mat_b");
}

TEST(Assets, ConstMaterialAccessByIndex)
{
    Assets assets;
    assets.resizeMaterials(1);
    assets.material(0).name("const_test");

    const auto& constAssets = assets;
    EXPECT_EQ(constAssets.material(0).name(), "const_test");
}

// ---------------------------------------------------------------------------
// Geometry access (CPU-side, no Renderer needed)
// ---------------------------------------------------------------------------

TEST(Assets, GeometryAccessSetVerticesAndIndices)
{
    Assets assets;
    assets.resizeGeometries(1);

    std::vector<Vertex> verts = {
        {{0, 0, 0}, {1, 1, 1}, {0, 1, 0}, {0, 0}},
        {{1, 0, 0}, {1, 1, 1}, {0, 1, 0}, {1, 0}},
        {{0, 1, 0}, {1, 1, 1}, {0, 1, 0}, {0, 1}},
    };
    std::vector<uint16_t> idxs = {0, 1, 2};

    assets.geometry(0).vertices(verts);
    assets.geometry(0).indices(idxs);

    EXPECT_EQ(assets.geometry(0).vertices().size(), 3);
    EXPECT_EQ(assets.geometry(0).indices().size(), 3);
    EXPECT_EQ(assets.geometry(0).indexCount(), 3);
}

TEST(Assets, GeometryDefaultNotLoaded)
{
    Assets assets;
    assets.resizeGeometries(1);
    EXPECT_FALSE(assets.geometry(0).loaded());
}

// ---------------------------------------------------------------------------
// Pointer stability after resize
// ---------------------------------------------------------------------------

TEST(Assets, MaterialPointerStableAfterModification)
{
    Assets assets;
    assets.resizeMaterials(3);

    Material* ptr = &assets.material(1);
    ptr->name("stable_test");

    EXPECT_EQ(assets.material(1).name(), "stable_test");
    EXPECT_EQ(ptr, &assets.material(1));
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

TEST(Assets, MoveConstructionTransfersOwnership)
{
    Assets original;
    original.resizeMaterials(2);
    original.resizeGeometries(3);
    original.material(0).name("moved");

    Assets moved(std::move(original));

    EXPECT_EQ(moved.materialCount(), 2);
    EXPECT_EQ(moved.geometryCount(), 3);
    EXPECT_EQ(moved.material(0).name(), "moved");
}

TEST(Assets, MoveAssignmentTransfersOwnership)
{
    Assets original;
    original.resizeMaterials(1);
    original.material(0).name("assigned");

    Assets target;
    target = std::move(original);

    EXPECT_EQ(target.materialCount(), 1);
    EXPECT_EQ(target.material(0).name(), "assigned");
}

// ---------------------------------------------------------------------------
// Non-copyable
// ---------------------------------------------------------------------------

TEST(Assets, IsNonCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Assets>);
    EXPECT_FALSE(std::is_copy_assignable_v<Assets>);
}
