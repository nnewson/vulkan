#include <fire_engine/scene/scene_graph.hpp>

#include <gtest/gtest.h>

using fire_engine::Mat4;
using fire_engine::Node;
using fire_engine::SceneGraph;

// ==========================================================================
// Construction
// ==========================================================================

TEST(SceneGraphConstruction, DefaultHasNoNodes)
{
    SceneGraph sg;
    EXPECT_TRUE(sg.nodes().empty());
}

TEST(SceneGraphConstruction, DefaultRootTransformIsIdentity)
{
    SceneGraph sg;
    EXPECT_EQ(sg.rootTransform(), Mat4::identity());
}

// ==========================================================================
// Adding Nodes
// ==========================================================================

TEST(SceneGraphNodes, AddNodeReturnsReference)
{
    SceneGraph sg;
    auto node = std::make_unique<Node>("Test");
    Node& ref = sg.addNode(std::move(node));
    EXPECT_EQ(ref.name(), "Test");
}

TEST(SceneGraphNodes, AddNodeIncreasesCount)
{
    SceneGraph sg;
    sg.addNode(std::make_unique<Node>("A"));
    sg.addNode(std::make_unique<Node>("B"));
    EXPECT_EQ(sg.nodes().size(), 2u);
}

TEST(SceneGraphNodes, NodesPreserveOrder)
{
    SceneGraph sg;
    sg.addNode(std::make_unique<Node>("First"));
    sg.addNode(std::make_unique<Node>("Second"));
    sg.addNode(std::make_unique<Node>("Third"));

    EXPECT_EQ(sg.nodes()[0]->name(), "First");
    EXPECT_EQ(sg.nodes()[1]->name(), "Second");
    EXPECT_EQ(sg.nodes()[2]->name(), "Third");
}

// ==========================================================================
// Root Transform
// ==========================================================================

TEST(SceneGraphRootTransform, SetRootTransform)
{
    SceneGraph sg;
    Mat4 t = Mat4::translate({10.0f, 20.0f, 30.0f});
    sg.rootTransform(t);
    EXPECT_EQ(sg.rootTransform(), t);
}

// ==========================================================================
// Move Semantics
// ==========================================================================

TEST(SceneGraphMove, MoveConstructTransfersNodes)
{
    SceneGraph a;
    a.addNode(std::make_unique<Node>("N1"));
    a.addNode(std::make_unique<Node>("N2"));

    SceneGraph b{std::move(a)};
    EXPECT_EQ(b.nodes().size(), 2u);
    EXPECT_EQ(b.nodes()[0]->name(), "N1");
}

TEST(SceneGraphMove, MoveConstructTransfersRootTransform)
{
    SceneGraph a;
    Mat4 t = Mat4::scale({2.0f, 2.0f, 2.0f});
    a.rootTransform(t);

    SceneGraph b{std::move(a)};
    EXPECT_EQ(b.rootTransform(), t);
}
