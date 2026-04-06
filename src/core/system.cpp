#include <fire_engine/core/system.hpp>

#include <GLFW/glfw3.h>

namespace fire_engine
{

void System::init()
{
    glfwInit();
}

void System::destroy()
{
    glfwTerminate();
}

double System::getTime()
{
    return glfwGetTime();
}

} // namespace fire_engine
