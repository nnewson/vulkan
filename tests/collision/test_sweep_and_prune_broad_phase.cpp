#include <fire_engine/collision/sweep_and_prune_broad_phase.hpp>

#include <gtest/gtest.h>

using fire_engine::AABB;
using fire_engine::Collider;
using fire_engine::ColliderId;
using fire_engine::EndPoint;
using fire_engine::Mat4;
using fire_engine::SweepAndPruneBroadPhase;
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

bool containsPair(const std::vector<fire_engine::CollisionPair>& pairs, const Collider& first,
                  const Collider& second)
{
    for (const fire_engine::CollisionPair& pair : pairs)
    {
        if ((pair.first == &first && pair.second == &second) ||
            (pair.first == &second && pair.second == &first))
        {
            return true;
        }
    }

    return false;
}

} // namespace

TEST(ColliderUpdate, TranslatesLocalBoundsAsPositions)
{
    Collider collider = makeCollider({0.0f, 1.0f, 2.0f}, {1.0f, 2.0f, 3.0f},
                                     Mat4::translate({10.0f, 20.0f, 30.0f}));

    AABB bounds = collider.worldBounds();
    EXPECT_FLOAT_EQ(bounds.min.x(), 10.0f);
    EXPECT_FLOAT_EQ(bounds.min.y(), 21.0f);
    EXPECT_FLOAT_EQ(bounds.min.z(), 32.0f);
    EXPECT_FLOAT_EQ(bounds.max.x(), 11.0f);
    EXPECT_FLOAT_EQ(bounds.max.y(), 22.0f);
    EXPECT_FLOAT_EQ(bounds.max.z(), 33.0f);
}

TEST(ColliderUpdate, RebuildsConservativeBoundsForNegativeScale)
{
    Collider collider =
        makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f}, Mat4::scale({-2.0f, 3.0f, -4.0f}));

    AABB bounds = collider.worldBounds();
    EXPECT_FLOAT_EQ(bounds.min.x(), -2.0f);
    EXPECT_FLOAT_EQ(bounds.min.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.min.z(), -12.0f);
    EXPECT_FLOAT_EQ(bounds.max.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.max.y(), 6.0f);
    EXPECT_FLOAT_EQ(bounds.max.z(), 0.0f);
}

TEST(ColliderUpdate, FirstUpdateSeedsPreviousAndSweptBoundsToCurrent)
{
    Collider collider =
        makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, Mat4::translate({2.0f, 0.0f, 0.0f}));

    EXPECT_EQ(collider.previousWorldBounds().min, Vec3(2.0f, 0.0f, 0.0f));
    EXPECT_EQ(collider.previousWorldBounds().max, Vec3(3.0f, 1.0f, 1.0f));
    EXPECT_EQ(collider.sweptWorldBounds().min, Vec3(2.0f, 0.0f, 0.0f));
    EXPECT_EQ(collider.sweptWorldBounds().max, Vec3(3.0f, 1.0f, 1.0f));
}

TEST(ColliderUpdate, SweptBoundsMergePreviousAndCurrentBounds)
{
    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    collider.update(Mat4::translate({3.0f, 0.0f, 0.0f}));

    EXPECT_EQ(collider.previousWorldBounds().min, Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(collider.previousWorldBounds().max, Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_EQ(collider.worldBounds().min, Vec3(3.0f, 0.0f, 0.0f));
    EXPECT_EQ(collider.worldBounds().max, Vec3(4.0f, 1.0f, 1.0f));
    EXPECT_EQ(collider.sweptWorldBounds().min, Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(collider.sweptWorldBounds().max, Vec3(4.0f, 1.0f, 1.0f));
}

TEST(SweepAndPruneBroadPhase, EmptyAndSingleColliderProduceNoPairs)
{
    SweepAndPruneBroadPhase broadPhase;
    broadPhase.update();
    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());

    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    ColliderId id = broadPhase.addCollider(collider);
    broadPhase.update();

    EXPECT_TRUE(id.valid());
    EXPECT_EQ(collider.colliderId(), id);
    EXPECT_EQ(broadPhase.colliderCount(), 1U);
    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, OverlapOnAllAxesProducesPossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    ColliderId firstId = broadPhase.addCollider(first);
    ColliderId secondId = broadPhase.addCollider(second);
    broadPhase.update();

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_EQ(broadPhase.possiblePairs()[0].firstId, firstId);
    EXPECT_EQ(broadPhase.possiblePairs()[0].secondId, secondId);
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, AddColliderReturnsStableIdsAndAssignsEndPointIndices)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    ColliderId firstId = broadPhase.addCollider(first);
    ColliderId secondId = broadPhase.addCollider(second);

    EXPECT_TRUE(firstId.valid());
    EXPECT_TRUE(secondId.valid());
    EXPECT_NE(firstId, secondId);
    EXPECT_EQ(first.colliderId(), firstId);
    EXPECT_EQ(second.colliderId(), secondId);

    for (EndPoint* endPoint : first.endPoints())
    {
        EXPECT_NE(endPoint->index(), EndPoint::invalidIndex);
        EXPECT_EQ(endPoint->colliderId(), firstId);
    }

    for (EndPoint* endPoint : second.endPoints())
    {
        EXPECT_NE(endPoint->index(), EndPoint::invalidIndex);
        EXPECT_EQ(endPoint->colliderId(), secondId);
    }

    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, OverlapOnOnlyTwoAxesIsPruned)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 3.0f}, {3.0f, 3.0f, 4.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    broadPhase.update();

    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, TouchingBoundsProducePossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({1.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 1.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    broadPhase.update();

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, UpdateColliderIncrementallyAddsPossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({3.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 1.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    ASSERT_TRUE(broadPhase.possiblePairs().empty());

    second.update(Mat4::translate({-2.5f, 0.0f, 0.0f}));
    broadPhase.updateCollider(second);

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, UpdateColliderIncrementallyRemovesPossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);

    second.update(Mat4::translate({5.0f, 0.0f, 0.0f}));
    broadPhase.updateCollider(second);

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_TRUE(broadPhase.validate());

    second.resetFrame(Mat4::translate({5.0f, 0.0f, 0.0f}));
    broadPhase.updateCollider(second);

    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, UpdateIncrementallyProcessesAllRegisteredColliders)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({3.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 1.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    ASSERT_TRUE(broadPhase.possiblePairs().empty());

    second.update(Mat4::translate({-2.5f, 0.0f, 0.0f}));
    broadPhase.update();

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, SweptBoundsProducePossiblePairForFastPassThrough)
{
    Collider moving = makeCollider({-3.0f, 0.0f, 0.0f}, {-2.0f, 1.0f, 1.0f});
    Collider target = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(moving);
    broadPhase.addCollider(target);
    ASSERT_TRUE(broadPhase.possiblePairs().empty());

    moving.update(Mat4::translate({5.0f, 0.0f, 0.0f}));
    broadPhase.updateCollider(moving);

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), moving, target));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, RebuildSynchronisesStateAfterFilterChange)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);

    first.collisionMask(0U);
    broadPhase.rebuild();

    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, CollisionMasksFilterPairs)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});
    first.collisionLayer(1U << 0U);
    first.collisionMask(1U << 1U);
    second.collisionLayer(1U << 2U);
    second.collisionMask(1U << 0U);

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);

    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());

    second.collisionLayer(1U << 1U);
    broadPhase.rebuild();

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, MultipleCollidersOnlyReturnMultiaxisOverlaps)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});
    Collider third = makeCollider({4.0f, 4.0f, 4.0f}, {5.0f, 5.0f, 5.0f});
    Collider fourth = makeCollider({1.0f, 1.0f, 3.1f}, {3.0f, 3.0f, 4.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    broadPhase.addCollider(third);
    broadPhase.addCollider(fourth);
    broadPhase.update();

    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(broadPhase.possiblePairs(), first, second));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, DuplicateColliderIsIgnored)
{
    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    SweepAndPruneBroadPhase broadPhase;
    ColliderId firstAdd = broadPhase.addCollider(collider);
    ColliderId secondAdd = broadPhase.addCollider(collider);
    broadPhase.update();

    EXPECT_EQ(firstAdd, secondAdd);
    EXPECT_EQ(broadPhase.colliderCount(), 1U);
    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, RemoveColliderByIdClearsPairsAndEndpointIndices)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    ColliderId firstId = broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);

    EXPECT_TRUE(broadPhase.removeCollider(firstId));

    EXPECT_EQ(broadPhase.colliderCount(), 1U);
    EXPECT_FALSE(first.colliderId().valid());
    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    for (EndPoint* endPoint : first.endPoints())
    {
        EXPECT_EQ(endPoint->index(), EndPoint::invalidIndex);
        EXPECT_FALSE(endPoint->colliderId().valid());
    }
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, RemoveColliderByReferenceClearsPairs)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);

    EXPECT_TRUE(broadPhase.removeCollider(second));

    EXPECT_EQ(broadPhase.colliderCount(), 1U);
    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, RemoveUnregisteredColliderReturnsFalse)
{
    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    SweepAndPruneBroadPhase broadPhase;

    EXPECT_FALSE(broadPhase.removeCollider(collider));
    EXPECT_FALSE(broadPhase.removeCollider(ColliderId{123U}));
    EXPECT_TRUE(broadPhase.validate());
}

TEST(SweepAndPruneBroadPhase, ClearRemovesCollidersAndPairs)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    SweepAndPruneBroadPhase broadPhase;
    broadPhase.addCollider(first);
    broadPhase.addCollider(second);
    broadPhase.update();
    ASSERT_EQ(broadPhase.possiblePairs().size(), 1U);

    broadPhase.clear();

    EXPECT_EQ(broadPhase.colliderCount(), 0U);
    EXPECT_FALSE(first.colliderId().valid());
    EXPECT_FALSE(second.colliderId().valid());
    EXPECT_TRUE(broadPhase.possiblePairs().empty());
    EXPECT_TRUE(broadPhase.validate());
}
