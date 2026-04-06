#pragma once

#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

namespace fire_engine
{

class Window
{
public:
    explicit Window(size_t width, size_t height, std::string_view title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = default;

    [[nodiscard]]
    bool shouldClose() const noexcept
    {
        return glfwWindowShouldClose(window_);
    }

    [[nodiscard]]
    GLFWwindow* getWindow() const noexcept
    {
        return window_;
    }

    [[nodiscard]]
    bool framebufferResized() const noexcept
    {
        return framebufferResized_;
    }

    void framebufferResized(bool resized) noexcept
    {
        framebufferResized_ = resized;
    }

private:
    GLFWwindow* window_ = nullptr;
    bool framebufferResized_ = false;
};

} // namespace fire_engine
