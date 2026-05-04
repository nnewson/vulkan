#include <fire_engine/input/variant_state.hpp>

#include <gtest/gtest.h>

#include <type_traits>

using fire_engine::VariantState;

TEST(VariantStateConstruction, DefaultHasNoCycleCommand)
{
    VariantState s;
    EXPECT_EQ(s.cycleDelta(), 0);
    EXPECT_FALSE(s.hasCycleCommand());
}

TEST(VariantStateAccessors, PositiveCycleRoundTrip)
{
    VariantState s;
    s.cycleDelta(1);
    EXPECT_EQ(s.cycleDelta(), 1);
    EXPECT_TRUE(s.hasCycleCommand());
}

TEST(VariantStateAccessors, NegativeCycleRoundTrip)
{
    VariantState s;
    s.cycleDelta(-1);
    EXPECT_EQ(s.cycleDelta(), -1);
    EXPECT_TRUE(s.hasCycleCommand());
}

TEST(VariantStateNoexcept, OperationsAreNoexcept)
{
    VariantState s;

    static_assert(noexcept(VariantState{}));
    static_assert(noexcept(s.cycleDelta()));
    static_assert(noexcept(s.cycleDelta(0)));
    static_assert(noexcept(s.hasCycleCommand()));
    static_assert(std::is_nothrow_move_constructible_v<VariantState>);
}
