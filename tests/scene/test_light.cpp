#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <fire_engine/graphics/colour3.hpp>
#include <fire_engine/graphics/lighting.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/render/render_context.hpp>
#include <fire_engine/scene/components.hpp>
#include <fire_engine/scene/light.hpp>

using fire_engine::Colour3;
using fire_engine::componentName;
using fire_engine::Components;
using fire_engine::Light;
using fire_engine::Lighting;
using fire_engine::Mat4;
using fire_engine::RenderContext;
using fire_engine::Vec3;

TEST(Light, DefaultsAreDirectionalWhiteUnit)
{
    Light l;
    EXPECT_EQ(l.type(), Light::Type::Directional);
    EXPECT_EQ(l.colour(), Colour3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(l.intensity(), 1.0f);
    EXPECT_FLOAT_EQ(l.range(), 0.0f);
}

TEST(Light, TypeRoundTrip)
{
    Light l;
    l.type(Light::Type::Point);
    EXPECT_EQ(l.type(), Light::Type::Point);
    l.type(Light::Type::Spot);
    EXPECT_EQ(l.type(), Light::Type::Spot);
    l.type(Light::Type::Directional);
    EXPECT_EQ(l.type(), Light::Type::Directional);
}

TEST(Light, ColourRoundTrip)
{
    Light l;
    l.colour(Colour3(0.2f, 0.5f, 0.8f));
    EXPECT_EQ(l.colour(), Colour3(0.2f, 0.5f, 0.8f));
}

TEST(Light, IntensityRoundTrip)
{
    Light l;
    l.intensity(12.5f);
    EXPECT_FLOAT_EQ(l.intensity(), 12.5f);
}

TEST(Light, RangeRoundTrip)
{
    Light l;
    l.range(7.5f);
    EXPECT_FLOAT_EQ(l.range(), 7.5f);
}

TEST(Light, ConeAnglesDefaultMatchKhrPunctualSensible)
{
    Light l;
    EXPECT_FLOAT_EQ(l.innerConeRad(), fire_engine::pi / 8.0f);
    EXPECT_FLOAT_EQ(l.outerConeRad(), fire_engine::pi / 4.0f);
}

TEST(Light, OuterConeStaysGreaterOrEqualToInnerWhenInnerIncreases)
{
    Light l;
    l.outerConeRad(fire_engine::pi / 6.0f);
    l.innerConeRad(fire_engine::pi / 3.0f);
    EXPECT_FLOAT_EQ(l.innerConeRad(), fire_engine::pi / 3.0f);
    EXPECT_FLOAT_EQ(l.outerConeRad(), fire_engine::pi / 3.0f);
}

TEST(Light, InnerConeStaysLessOrEqualToOuterWhenOuterDecreases)
{
    Light l;
    l.innerConeRad(fire_engine::pi / 3.0f);
    l.outerConeRad(fire_engine::pi / 6.0f);
    EXPECT_FLOAT_EQ(l.innerConeRad(), fire_engine::pi / 6.0f);
    EXPECT_FLOAT_EQ(l.outerConeRad(), fire_engine::pi / 6.0f);
}

TEST(Components, LightVariantNameIsLight)
{
    Components c = Light{};
    EXPECT_EQ(componentName(c), "Light");
}

// ---------------------------------------------------------------------------
// E2: gather pass — Light::toLighting resolves world-space data
// ---------------------------------------------------------------------------

TEST(LightGather, IdentityWorldDirectionalForwardIsNegativeZ)
{
    Light l;
    l.type(Light::Type::Directional);
    auto inst = Light::toLighting(l, Mat4::identity());
    EXPECT_EQ(inst.type, 0);
    EXPECT_FLOAT_EQ(inst.worldPosition.x(), 0.0f);
    EXPECT_FLOAT_EQ(inst.worldPosition.y(), 0.0f);
    EXPECT_FLOAT_EQ(inst.worldPosition.z(), 0.0f);
    EXPECT_FLOAT_EQ(inst.worldDirection.x(), 0.0f);
    EXPECT_FLOAT_EQ(inst.worldDirection.y(), 0.0f);
    EXPECT_FLOAT_EQ(inst.worldDirection.z(), -1.0f);
}

TEST(LightGather, TranslatedPointKeepsForwardSetsPosition)
{
    Light l;
    l.type(Light::Type::Point);
    l.range(8.0f);
    auto world = Mat4::translate(Vec3{3.0f, 4.0f, 5.0f});
    auto inst = Light::toLighting(l, world);
    EXPECT_EQ(inst.type, 1);
    EXPECT_FLOAT_EQ(inst.worldPosition.x(), 3.0f);
    EXPECT_FLOAT_EQ(inst.worldPosition.y(), 4.0f);
    EXPECT_FLOAT_EQ(inst.worldPosition.z(), 5.0f);
    EXPECT_FLOAT_EQ(inst.range, 8.0f);
}

TEST(LightGather, RotatedNodeRotatesForward)
{
    // Yaw 90° → forward rotates from -Z to -X (matches glTF camera convention).
    Light l;
    l.type(Light::Type::Spot);
    auto world = Mat4::rotateY(fire_engine::pi * 0.5f);
    auto inst = Light::toLighting(l, world);
    EXPECT_EQ(inst.type, 2);
    EXPECT_NEAR(inst.worldDirection.x(), -1.0f, 1e-5f);
    EXPECT_NEAR(inst.worldDirection.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(inst.worldDirection.z(), 0.0f, 1e-5f);
}

TEST(LightGather, ConeAnglesPackAsCosines)
{
    Light l;
    l.type(Light::Type::Spot);
    l.outerConeRad(fire_engine::pi / 3.0f);
    l.innerConeRad(fire_engine::pi / 6.0f);
    auto inst = Light::toLighting(l, Mat4::identity());
    EXPECT_NEAR(inst.innerConeCos, std::cos(fire_engine::pi / 6.0f), 1e-6f);
    EXPECT_NEAR(inst.outerConeCos, std::cos(fire_engine::pi / 3.0f), 1e-6f);
}

TEST(LightGather, ColourAndIntensityRoundTrip)
{
    Light l;
    l.colour(Colour3(0.2f, 0.4f, 0.6f));
    l.intensity(7.5f);
    auto inst = Light::toLighting(l, Mat4::identity());
    EXPECT_EQ(inst.colour, Colour3(0.2f, 0.4f, 0.6f));
    EXPECT_FLOAT_EQ(inst.intensity, 7.5f);
}

TEST(LightGather, ScaledNodeStillEmitsUnitForward)
{
    Light l;
    auto world = Mat4::scale(Vec3{4.0f, 4.0f, 4.0f});
    auto inst = Light::toLighting(l, world);
    EXPECT_NEAR(inst.worldDirection.magnitude(), 1.0f, 1e-5f);
    EXPECT_NEAR(inst.worldDirection.z(), -1.0f, 1e-5f);
}

// ---------------------------------------------------------------------------
// E3: SceneGraph::gatherLights walks the tree and resolves world-space light
// data from each Light component's node.composedWorld().
// ---------------------------------------------------------------------------

#include <fire_engine/input/input_state.hpp>
#include <fire_engine/scene/node.hpp>
#include <fire_engine/scene/scene_graph.hpp>

using fire_engine::InputState;
using fire_engine::Node;
using fire_engine::SceneGraph;

TEST(GatherLights, EmptySceneReturnsNoLights)
{
    SceneGraph scene;
    auto lights = scene.gatherLights();
    EXPECT_TRUE(lights.empty());
}

TEST(GatherLights, SingleDirectionalAtRoot)
{
    SceneGraph scene;
    auto node = std::make_unique<Node>("Sun");
    node->component().emplace<Light>().intensity(2.5f);
    scene.addNode(std::move(node));

    InputState input;
    scene.update(input);

    auto lights = scene.gatherLights();
    ASSERT_EQ(lights.size(), 1u);
    EXPECT_EQ(lights[0].type, 0);
    EXPECT_FLOAT_EQ(lights[0].intensity, 2.5f);
}

TEST(GatherLights, NestedLightUsesComposedWorldPosition)
{
    SceneGraph scene;
    auto root = std::make_unique<Node>("Root");
    root->transform().position(Vec3{2.0f, 0.0f, 0.0f});
    auto child = std::make_unique<Node>("Lamp");
    child->transform().position(Vec3{0.0f, 3.0f, 0.0f});
    auto& childLight = child->component().emplace<Light>();
    childLight.type(Light::Type::Point);
    childLight.range(5.0f);
    root->addChild(std::move(child));
    scene.addNode(std::move(root));

    InputState input;
    scene.update(input);

    auto lights = scene.gatherLights();
    ASSERT_EQ(lights.size(), 1u);
    EXPECT_EQ(lights[0].type, 1);
    EXPECT_FLOAT_EQ(lights[0].worldPosition.x(), 2.0f);
    EXPECT_FLOAT_EQ(lights[0].worldPosition.y(), 3.0f);
    EXPECT_FLOAT_EQ(lights[0].worldPosition.z(), 0.0f);
    EXPECT_FLOAT_EQ(lights[0].range, 5.0f);
}

TEST(GatherLights, MultipleLightsPreserveTraversalOrder)
{
    SceneGraph scene;
    auto a = std::make_unique<Node>("A");
    a->component().emplace<Light>().intensity(1.0f);
    auto b = std::make_unique<Node>("B");
    auto& bl = b->component().emplace<Light>();
    bl.type(Light::Type::Spot);
    bl.intensity(2.0f);
    scene.addNode(std::move(a));
    scene.addNode(std::move(b));

    InputState input;
    scene.update(input);

    auto lights = scene.gatherLights();
    ASSERT_EQ(lights.size(), 2u);
    EXPECT_FLOAT_EQ(lights[0].intensity, 1.0f);
    EXPECT_FLOAT_EQ(lights[1].intensity, 2.0f);
    EXPECT_EQ(lights[1].type, 2);
}

TEST(GatherLights, NonLightNodesAreSkipped)
{
    SceneGraph scene;
    auto a = std::make_unique<Node>("Empty"); // default Empty component
    scene.addNode(std::move(a));
    auto b = std::make_unique<Node>("Light");
    b->component().emplace<Light>();
    scene.addNode(std::move(b));

    InputState input;
    scene.update(input);

    auto lights = scene.gatherLights();
    EXPECT_EQ(lights.size(), 1u);
}

// ---------------------------------------------------------------------------
// D1 / E3: hasDirectionalLight — used by FireEngine to decide whether to seed
// the default Sun when KHR_lights_punctual already authored a directional.
// ---------------------------------------------------------------------------

TEST(HasDirectionalLight, EmptySceneReturnsFalse)
{
    SceneGraph scene;
    EXPECT_FALSE(scene.hasDirectionalLight());
}

TEST(HasDirectionalLight, SceneWithOnlyPointLightReturnsFalse)
{
    SceneGraph scene;
    auto node = std::make_unique<Node>("Lamp");
    auto& l = node->component().emplace<Light>();
    l.type(Light::Type::Point);
    scene.addNode(std::move(node));
    EXPECT_FALSE(scene.hasDirectionalLight());
}

TEST(HasDirectionalLight, SceneWithDirectionalReturnsTrue)
{
    SceneGraph scene;
    auto node = std::make_unique<Node>("Sun");
    node->component().emplace<Light>(); // default type = Directional
    scene.addNode(std::move(node));
    EXPECT_TRUE(scene.hasDirectionalLight());
}

TEST(HasDirectionalLight, FindsDirectionalNestedAsChild)
{
    SceneGraph scene;
    auto root = std::make_unique<Node>("Root");
    auto child = std::make_unique<Node>("Sun");
    child->component().emplace<Light>();
    root->addChild(std::move(child));
    scene.addNode(std::move(root));
    EXPECT_TRUE(scene.hasDirectionalLight());
}
