#pragma once

namespace fire_engine
{

class System
{
public:
    System() = delete;

    static void init();
    static void destroy();

    [[nodiscard]]
    static double getTime();
};

} // namespace fire_engine
