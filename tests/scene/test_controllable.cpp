#include <fire_engine/input/controller_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/scene/controllable.hpp>
#include <fire_engine/scene/transform.hpp>

#include <gtest/gtest.h>

using fire_engine::Controllable;
using fire_engine::ControllerState;
using fire_engine::Mat4;
using fire_engine::Transform;
using fire_engine::Vec3;

TEST(ControllableUpdate, AppliesControllerDeltaToTransformPosition)
{
    Controllable controllable;
    Transform transform;
    ControllerState state;
    state.deltaPosition({0.25f, 0.0f, 0.0f});

    controllable.update(state, transform, Mat4::identity());

    EXPECT_FLOAT_EQ(transform.position().x(), 2.5f);
    EXPECT_FLOAT_EQ(transform.position().y(), 0.0f);
    EXPECT_FLOAT_EQ(transform.position().z(), 0.0f);
    EXPECT_FLOAT_EQ((transform.world()[0, 3]), 2.5f);
}

TEST(ControllableUpdate, UsesParentWorldWhenUpdatingTransform)
{
    Controllable controllable;
    Transform transform;
    ControllerState state;
    state.deltaPosition({0.5f, 0.0f, 0.0f});

    controllable.update(state, transform, Mat4::translate(Vec3{1.0f, 2.0f, 3.0f}));

    EXPECT_FLOAT_EQ(transform.position().x(), 5.0f);
    EXPECT_FLOAT_EQ((transform.world()[0, 3]), 6.0f);
    EXPECT_FLOAT_EQ((transform.world()[1, 3]), 2.0f);
    EXPECT_FLOAT_EQ((transform.world()[2, 3]), 3.0f);
}
