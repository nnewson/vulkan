#include <fire_engine/platform/window.hpp>

namespace fire_engine
{

Window::Window(size_t width, size_t height, std::string_view title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* w, int, int)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        self->framebufferResized_ = true;
    });
}

Window::~Window()
{
    glfwDestroyWindow(window_);
}

} // namespace fire_engine
