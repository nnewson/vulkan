#include <fire_engine/scene/transform.hpp>

#include <gtest/gtest.h>

#include <fire_engine/math/quaternion.hpp>

using fire_engine::Mat4;
using fire_engine::Quaternion;
using fire_engine::Transform;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(TransformConstruction, DefaultPosition)
{
    Transform t;
    EXPECT_FLOAT_EQ(t.position().x(), 0.0f);
    EXPECT_FLOAT_EQ(t.position().y(), 0.0f);
    EXPECT_FLOAT_EQ(t.position().z(), 0.0f);
}

TEST(TransformConstruction, DefaultRotation)
{
    Transform t;
    EXPECT_FLOAT_EQ(t.rotation().x(), 0.0f);
    EXPECT_FLOAT_EQ(t.rotation().y(), 0.0f);
    EXPECT_FLOAT_EQ(t.rotation().z(), 0.0f);
    EXPECT_FLOAT_EQ(t.rotation().w(), 1.0f);
}

TEST(TransformConstruction, DefaultScale)
{
    Transform t;
    EXPECT_FLOAT_EQ(t.scale().x(), 1.0f);
    EXPECT_FLOAT_EQ(t.scale().y(), 1.0f);
    EXPECT_FLOAT_EQ(t.scale().z(), 1.0f);
}

TEST(TransformConstruction, DefaultLocalIsIdentity)
{
    Transform t;
    EXPECT_EQ(t.local(), Mat4::identity());
}

TEST(TransformConstruction, DefaultWorldIsIdentity)
{
    Transform t;
    EXPECT_EQ(t.world(), Mat4::identity());
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(TransformAccessors, SetPosition)
{
    Transform t;
    t.position({1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(t.position().x(), 1.0f);
    EXPECT_FLOAT_EQ(t.position().y(), 2.0f);
    EXPECT_FLOAT_EQ(t.position().z(), 3.0f);
}

TEST(TransformAccessors, SetRotation)
{
    Transform t;
    t.rotation(Quaternion{0.1f, 0.2f, 0.3f, 0.4f});
    EXPECT_FLOAT_EQ(t.rotation().x(), 0.1f);
    EXPECT_FLOAT_EQ(t.rotation().y(), 0.2f);
    EXPECT_FLOAT_EQ(t.rotation().z(), 0.3f);
    EXPECT_FLOAT_EQ(t.rotation().w(), 0.4f);
}

TEST(TransformAccessors, SetScale)
{
    Transform t;
    t.scale({2.0f, 3.0f, 4.0f});
    EXPECT_FLOAT_EQ(t.scale().x(), 2.0f);
    EXPECT_FLOAT_EQ(t.scale().y(), 3.0f);
    EXPECT_FLOAT_EQ(t.scale().z(), 4.0f);
}

// ==========================================================================
// Update with identity parent
// ==========================================================================

TEST(TransformUpdate, DefaultWithIdentityParentProducesIdentity)
{
    Transform t;
    t.update(Mat4::identity());
    EXPECT_EQ(t.local(), Mat4::identity());
    EXPECT_EQ(t.world(), Mat4::identity());
}

TEST(TransformUpdate, TranslationAffectsLocalMatrix)
{
    Transform t;
    t.position({5.0f, 10.0f, 15.0f});
    t.update(Mat4::identity());

    // Column-major: translation is in column 3
    EXPECT_FLOAT_EQ((t.local()[0, 3]), 5.0f);
    EXPECT_FLOAT_EQ((t.local()[1, 3]), 10.0f);
    EXPECT_FLOAT_EQ((t.local()[2, 3]), 15.0f);
}

TEST(TransformUpdate, ScaleAffectsLocalMatrix)
{
    Transform t;
    t.scale({2.0f, 3.0f, 4.0f});
    t.update(Mat4::identity());

    EXPECT_FLOAT_EQ((t.local()[0, 0]), 2.0f);
    EXPECT_FLOAT_EQ((t.local()[1, 1]), 3.0f);
    EXPECT_FLOAT_EQ((t.local()[2, 2]), 4.0f);
}

TEST(TransformUpdate, WorldEqualsLocalWithIdentityParent)
{
    Transform t;
    t.position({1.0f, 2.0f, 3.0f});
    t.scale({2.0f, 2.0f, 2.0f});
    t.update(Mat4::identity());

    EXPECT_EQ(t.world(), t.local());
}

// ==========================================================================
// Update with non-identity parent
// ==========================================================================

TEST(TransformUpdate, ParentTranslationAddsToChild)
{
    Mat4 parent = Mat4::translate({10.0f, 20.0f, 30.0f});

    Transform t;
    t.position({1.0f, 2.0f, 3.0f});
    t.update(parent);

    // World translation should be parent + child
    EXPECT_FLOAT_EQ((t.world()[0, 3]), 11.0f);
    EXPECT_FLOAT_EQ((t.world()[1, 3]), 22.0f);
    EXPECT_FLOAT_EQ((t.world()[2, 3]), 33.0f);
}

TEST(TransformUpdate, ParentScaleScalesChildTranslation)
{
    Mat4 parent = Mat4::scale({2.0f, 2.0f, 2.0f});

    Transform t;
    t.position({5.0f, 5.0f, 5.0f});
    t.update(parent);

    // Child position should be doubled by parent scale
    EXPECT_FLOAT_EQ((t.world()[0, 3]), 10.0f);
    EXPECT_FLOAT_EQ((t.world()[1, 3]), 10.0f);
    EXPECT_FLOAT_EQ((t.world()[2, 3]), 10.0f);
}

TEST(TransformUpdate, WorldIsParentTimesLocal)
{
    Mat4 parent = Mat4::translate({1.0f, 0.0f, 0.0f}) * Mat4::scale({2.0f, 2.0f, 2.0f});

    Transform t;
    t.position({3.0f, 4.0f, 5.0f});
    t.update(parent);

    Mat4 expected = parent * t.local();
    EXPECT_EQ(t.world(), expected);
}

// ==========================================================================
// Hierarchy simulation
// ==========================================================================

TEST(TransformHierarchy, ThreeLevelHierarchy)
{
    // Simulate grandparent → parent → child
    Transform grandparent;
    grandparent.position({10.0f, 0.0f, 0.0f});
    grandparent.update(Mat4::identity());

    Transform parent;
    parent.position({5.0f, 0.0f, 0.0f});
    parent.update(grandparent.world());

    Transform child;
    child.position({1.0f, 0.0f, 0.0f});
    child.update(parent.world());

    // Child's world X should be 10 + 5 + 1 = 16
    EXPECT_FLOAT_EQ((child.world()[0, 3]), 16.0f);
}

// ==========================================================================
// Copy and Move Semantics
// ==========================================================================

TEST(TransformCopy, CopyConstructCreatesIndependentCopy)
{
    Transform a;
    a.position({1.0f, 2.0f, 3.0f});
    a.update(Mat4::identity());

    Transform b{a};
    EXPECT_FLOAT_EQ(b.position().x(), 1.0f);
    EXPECT_EQ(b.local(), a.local());

    b.position({9.0f, 9.0f, 9.0f});
    EXPECT_FLOAT_EQ(a.position().x(), 1.0f);
}

TEST(TransformMove, MoveConstructTransfersState)
{
    Transform a;
    a.position({4.0f, 5.0f, 6.0f});
    a.update(Mat4::identity());
    Mat4 expectedLocal = a.local();

    Transform b{std::move(a)};
    EXPECT_FLOAT_EQ(b.position().x(), 4.0f);
    EXPECT_EQ(b.local(), expectedLocal);
}
