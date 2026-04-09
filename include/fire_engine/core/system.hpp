#pragma once

namespace fire_engine
{

struct GlfwBackend
{
    static void init();
    static void destroy();

    [[nodiscard]]
    static double getTime();
};

template <typename Backend>
class SystemT
{
public:
    SystemT() = delete;

    static void init()
    {
        Backend::init();
    }

    static void destroy()
    {
        Backend::destroy();
    }

    [[nodiscard]]
    static double getTime()
    {
        return Backend::getTime();
    }
};

using System = SystemT<GlfwBackend>;

} // namespace fire_engine
