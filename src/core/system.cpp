#include <fire_engine/core/system.hpp>

#include <GLFW/glfw3.h>

namespace fire_engine
{

void GlfwBackend::init()
{
    glfwInit();
}

void GlfwBackend::destroy()
{
    glfwTerminate();
}

double GlfwBackend::getTime()
{
    return glfwGetTime();
}

} // namespace fire_engine
