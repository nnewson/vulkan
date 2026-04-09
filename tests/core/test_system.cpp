#include <fire_engine/core/system.hpp>

#include <gtest/gtest.h>

using fire_engine::SystemT;

namespace
{

struct MockBackend
{
    static inline bool initCalled = false;
    static inline bool destroyCalled = false;
    static inline double mockTime = 0.0;

    static void init()
    {
        initCalled = true;
    }

    static void destroy()
    {
        destroyCalled = true;
    }

    static double getTime()
    {
        return mockTime;
    }

    static void reset()
    {
        initCalled = false;
        destroyCalled = false;
        mockTime = 0.0;
    }
};

using MockSystem = SystemT<MockBackend>;

} // namespace

// ==========================================================================
// Delegation
// ==========================================================================

TEST(SystemT, InitDelegatesToBackend)
{
    MockBackend::reset();
    EXPECT_FALSE(MockBackend::initCalled);
    MockSystem::init();
    EXPECT_TRUE(MockBackend::initCalled);
}

TEST(SystemT, DestroyDelegatesToBackend)
{
    MockBackend::reset();
    EXPECT_FALSE(MockBackend::destroyCalled);
    MockSystem::destroy();
    EXPECT_TRUE(MockBackend::destroyCalled);
}

TEST(SystemT, GetTimeDelegatesToBackend)
{
    MockBackend::reset();
    MockBackend::mockTime = 42.5;
    EXPECT_DOUBLE_EQ(MockSystem::getTime(), 42.5);
}

TEST(SystemT, GetTimeReflectsChanges)
{
    MockBackend::reset();
    MockBackend::mockTime = 1.0;
    EXPECT_DOUBLE_EQ(MockSystem::getTime(), 1.0);
    MockBackend::mockTime = 2.0;
    EXPECT_DOUBLE_EQ(MockSystem::getTime(), 2.0);
}

// ==========================================================================
// Non-instantiable
// ==========================================================================

TEST(SystemT, IsNotInstantiable)
{
    static_assert(!std::is_default_constructible_v<MockSystem>);
}
