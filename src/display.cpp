#include <fire_engine/display.hpp>

namespace fire_engine
{

Display::Display(size_t width, size_t height, std::string_view title,
                 GLFWframebuffersizefun framebufferResizeCallback)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

Display::~Display()
{
    glfwDestroyWindow(window_);
}

} // namespace fire_engine
