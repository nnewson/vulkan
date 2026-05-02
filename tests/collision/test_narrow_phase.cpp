#include <fire_engine/collision/narrow_phase.hpp>

#include <gtest/gtest.h>

using fire_engine::Collider;
using fire_engine::Mat4;
using fire_engine::NarrowPhase;
using fire_engine::Vec3;

namespace
{

Collider makeCollider(Vec3 min, Vec3 max, Mat4 world = Mat4::identity())
{
    Collider collider;
    collider.localBounds({min, max});
    collider.update(world);
    return collider;
}

} // namespace

TEST(SweptAabb, MovingBoxReportsTimeOfImpactAndNormal)
{
    NarrowPhase narrowPhase;
    Collider moving = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider target = makeCollider({3.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 1.0f});

    moving.update(Mat4::translate({4.0f, 0.0f, 0.0f}));

    auto contact = narrowPhase.sweptAabb(moving, target);
    ASSERT_TRUE(contact.has_value());
    EXPECT_FLOAT_EQ(contact->toi, 0.5f);
    EXPECT_EQ(contact->normal, Vec3(-1.0f, 0.0f, 0.0f));
}

TEST(SweptAabb, MissReturnsNoContact)
{
    NarrowPhase narrowPhase;
    Collider moving = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider target = makeCollider({3.0f, 3.0f, 0.0f}, {4.0f, 4.0f, 1.0f});

    moving.update(Mat4::translate({4.0f, 0.0f, 0.0f}));

    EXPECT_FALSE(narrowPhase.sweptAabb(moving, target).has_value());
}

TEST(SweptAabb, StartingOverlapReturnsImmediateContact)
{
    NarrowPhase narrowPhase;
    Collider moving = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider target = makeCollider({1.0f, 0.0f, 0.0f}, {3.0f, 2.0f, 2.0f});

    moving.update(Mat4::translate({1.0f, 0.0f, 0.0f}));

    auto contact = narrowPhase.sweptAabb(moving, target);
    ASSERT_TRUE(contact.has_value());
    EXPECT_FLOAT_EQ(contact->toi, 0.0f);
    EXPECT_EQ(contact->normal, Vec3(-1.0f, 0.0f, 0.0f));
}

TEST(SweptAabb, StartingTouchUsesGeometryNormalSoMovingAwayCanBeIgnored)
{
    NarrowPhase narrowPhase;
    Collider moving = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider target = makeCollider({1.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 1.0f});

    moving.update(Mat4::translate({-1.0f, 0.0f, 0.0f}));

    auto contact = narrowPhase.sweptAabb(moving, target);
    ASSERT_TRUE(contact.has_value());
    EXPECT_FLOAT_EQ(contact->toi, 0.0f);
    EXPECT_EQ(contact->normal, Vec3(-1.0f, 0.0f, 0.0f));
}
