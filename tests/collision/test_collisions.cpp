#include <fire_engine/collision/collisions.hpp>
#include <fire_engine/scene/scene_graph.hpp>

#include <memory>

#include <gtest/gtest.h>

using fire_engine::AABB;
using fire_engine::Collider;
using fire_engine::ColliderId;
using fire_engine::Collisions;
using fire_engine::EndPoint;
using fire_engine::InputState;
using fire_engine::Mat4;
using fire_engine::Node;
using fire_engine::SceneGraph;
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

TEST(Collisions, EmptyAndSingleColliderProduceNoPairs)
{
    Collisions collisions;
    collisions.update();
    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());

    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    ColliderId id = collisions.addCollider(collider);
    collisions.update();

    EXPECT_TRUE(id.valid());
    EXPECT_EQ(collider.colliderId(), id);
    EXPECT_EQ(collisions.colliderCount(), 1U);
    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, OverlapOnAllAxesProducesPossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    ColliderId firstId = collisions.addCollider(first);
    ColliderId secondId = collisions.addCollider(second);
    collisions.update();

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_EQ(collisions.possiblePairs()[0].firstId, firstId);
    EXPECT_EQ(collisions.possiblePairs()[0].secondId, secondId);
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, AddColliderReturnsStableIdsAndAssignsEndPointIndices)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    ColliderId firstId = collisions.addCollider(first);
    ColliderId secondId = collisions.addCollider(second);

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

    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, OverlapOnOnlyTwoAxesIsPruned)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 3.0f}, {3.0f, 3.0f, 4.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    collisions.update();

    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, TouchingBoundsProducePossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({1.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 1.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    collisions.update();

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, UpdateColliderIncrementallyAddsPossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({3.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 1.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    ASSERT_TRUE(collisions.possiblePairs().empty());

    second.update(Mat4::translate({-2.5f, 0.0f, 0.0f}));
    collisions.updateCollider(second);

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, UpdateColliderIncrementallyRemovesPossiblePair)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    ASSERT_EQ(collisions.possiblePairs().size(), 1U);

    second.update(Mat4::translate({5.0f, 0.0f, 0.0f}));
    collisions.updateCollider(second);

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_TRUE(collisions.validate());

    second.resetFrame(Mat4::translate({5.0f, 0.0f, 0.0f}));
    collisions.updateCollider(second);

    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, UpdateIncrementallyProcessesAllRegisteredColliders)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collider second = makeCollider({3.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 1.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    ASSERT_TRUE(collisions.possiblePairs().empty());

    second.update(Mat4::translate({-2.5f, 0.0f, 0.0f}));
    collisions.update();

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, SweptBoundsProducePossiblePairForFastPassThrough)
{
    Collider moving = makeCollider({-3.0f, 0.0f, 0.0f}, {-2.0f, 1.0f, 1.0f});
    Collider target = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    Collisions collisions;
    collisions.addCollider(moving);
    collisions.addCollider(target);
    ASSERT_TRUE(collisions.possiblePairs().empty());

    moving.update(Mat4::translate({5.0f, 0.0f, 0.0f}));
    collisions.updateCollider(moving);

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), moving, target));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, RebuildSynchronisesStateAfterFilterChange)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    ASSERT_EQ(collisions.possiblePairs().size(), 1U);

    first.collisionMask(0U);
    collisions.rebuild();

    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, CollisionMasksFilterPairs)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});
    first.collisionLayer(1U << 0U);
    first.collisionMask(1U << 1U);
    second.collisionLayer(1U << 2U);
    second.collisionMask(1U << 0U);

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);

    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());

    second.collisionLayer(1U << 1U);
    collisions.rebuild();

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, MultipleCollidersOnlyReturnMultiaxisOverlaps)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});
    Collider third = makeCollider({4.0f, 4.0f, 4.0f}, {5.0f, 5.0f, 5.0f});
    Collider fourth = makeCollider({1.0f, 1.0f, 3.1f}, {3.0f, 3.0f, 4.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    collisions.addCollider(third);
    collisions.addCollider(fourth);
    collisions.update();

    ASSERT_EQ(collisions.possiblePairs().size(), 1U);
    EXPECT_TRUE(containsPair(collisions.possiblePairs(), first, second));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, DuplicateColliderIsIgnored)
{
    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    Collisions collisions;
    ColliderId firstAdd = collisions.addCollider(collider);
    ColliderId secondAdd = collisions.addCollider(collider);
    collisions.update();

    EXPECT_EQ(firstAdd, secondAdd);
    EXPECT_EQ(collisions.colliderCount(), 1U);
    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, RemoveColliderByIdClearsPairsAndEndpointIndices)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    ColliderId firstId = collisions.addCollider(first);
    collisions.addCollider(second);
    ASSERT_EQ(collisions.possiblePairs().size(), 1U);

    EXPECT_TRUE(collisions.removeCollider(firstId));

    EXPECT_EQ(collisions.colliderCount(), 1U);
    EXPECT_FALSE(first.colliderId().valid());
    EXPECT_TRUE(collisions.possiblePairs().empty());
    for (EndPoint* endPoint : first.endPoints())
    {
        EXPECT_EQ(endPoint->index(), EndPoint::invalidIndex);
        EXPECT_FALSE(endPoint->colliderId().valid());
    }
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, RemoveColliderByReferenceClearsPairs)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    ASSERT_EQ(collisions.possiblePairs().size(), 1U);

    EXPECT_TRUE(collisions.removeCollider(second));

    EXPECT_EQ(collisions.colliderCount(), 1U);
    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, RemoveUnregisteredColliderReturnsFalse)
{
    Collider collider = makeCollider({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    Collisions collisions;

    EXPECT_FALSE(collisions.removeCollider(collider));
    EXPECT_FALSE(collisions.removeCollider(ColliderId{123U}));
    EXPECT_TRUE(collisions.validate());
}

TEST(Collisions, ClearRemovesCollidersAndPairs)
{
    Collider first = makeCollider({0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
    Collider second = makeCollider({1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f});

    Collisions collisions;
    collisions.addCollider(first);
    collisions.addCollider(second);
    collisions.update();
    ASSERT_EQ(collisions.possiblePairs().size(), 1U);

    collisions.clear();

    EXPECT_EQ(collisions.colliderCount(), 0U);
    EXPECT_FALSE(first.colliderId().valid());
    EXPECT_FALSE(second.colliderId().valid());
    EXPECT_TRUE(collisions.possiblePairs().empty());
    EXPECT_TRUE(collisions.validate());
}

TEST(CollisionsSystem, SetupRegistersSceneNodesWithColliders)
{
    SceneGraph scene;
    auto colliderNode = std::make_unique<Node>("Collider");
    colliderNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    scene.addNode(std::move(colliderNode));
    scene.addNode(std::make_unique<Node>("Empty"));

    Collisions collisions;
    collisions.setup(scene);

    EXPECT_EQ(collisions.colliderCount(), 1U);
    EXPECT_TRUE(collisions.validate());
}

TEST(CollisionsSystem, DynamicNodeReflectsAndSceneBoundsResolveAfterContact)
{
    SceneGraph scene;
    auto dynamicNode = std::make_unique<Node>("Dynamic");
    dynamicNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    dynamicNode->emplacePhysicsBody().velocity({4.0f, 0.0f, 0.0f});
    Node* dynamic = &scene.addNode(std::move(dynamicNode));

    auto wallNode = std::make_unique<Node>("Wall");
    wallNode->transform().position({3.0f, 0.0f, 0.0f});
    wallNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    scene.addNode(std::move(wallNode));

    Collisions collisions;
    collisions.setup(scene);

    InputState input;
    input.deltaTime(1.0f);
    scene.update(input);
    collisions.update(scene);

    ASSERT_NE(dynamic->physicsBody(), nullptr);
    ASSERT_NE(dynamic->collider(), nullptr);
    EXPECT_EQ(dynamic->transform().position(), Vec3(2.0f, 0.0f, 0.0f));
    EXPECT_EQ(dynamic->physicsBody()->velocity(), Vec3(-4.0f, 0.0f, 0.0f));
    EXPECT_EQ(dynamic->collider()->previousWorldBounds().min, Vec3(2.0f, 0.0f, 0.0f));
    EXPECT_EQ(dynamic->collider()->sweptWorldBounds().min, Vec3(2.0f, 0.0f, 0.0f));
}

TEST(CollisionsSystem, ControllableSlidesToWallImpact)
{
    SceneGraph scene;
    auto paddleNode = std::make_unique<Node>("Paddle");
    paddleNode->emplaceControllable();
    paddleNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    Node* paddle = &scene.addNode(std::move(paddleNode));

    auto wallNode = std::make_unique<Node>("Wall");
    wallNode->transform().position({3.0f, 0.0f, 0.0f});
    wallNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    scene.addNode(std::move(wallNode));

    Collisions collisions;
    collisions.setup(scene);

    InputState input;
    input.controllerState().deltaPosition({0.4f, 0.0f, 0.0f});
    scene.update(input);
    collisions.update(scene);

    EXPECT_EQ(paddle->transform().position(), Vec3(2.0f, 0.0f, 0.0f));
}

TEST(CollisionsSystem, ControllableCanMoveAwayFromTouchingWall)
{
    SceneGraph scene;
    auto paddleNode = std::make_unique<Node>("Paddle");
    paddleNode->emplaceControllable();
    paddleNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    Node* paddle = &scene.addNode(std::move(paddleNode));

    auto wallNode = std::make_unique<Node>("Wall");
    wallNode->transform().position({1.0f, 0.0f, 0.0f});
    wallNode->emplaceCollider().localBounds({Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}});
    scene.addNode(std::move(wallNode));

    Collisions collisions;
    collisions.setup(scene);

    InputState input;
    input.controllerState().deltaPosition({-1.0f, 0.0f, 0.0f});
    scene.update(input);
    collisions.update(scene);

    EXPECT_EQ(paddle->transform().position(), Vec3(-10.0f, 0.0f, 0.0f));
}
