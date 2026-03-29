#pragma once

#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

namespace fire_engine
{

class Display
{
public:
    explicit Display(size_t width, size_t height, const std::string_view title,
                     GLFWframebuffersizefun framebufferResizeCallback);
    ~Display();

    GLFWwindow* getWindow() const
    {
        return window_;
    }

private:
    GLFWwindow* window_ = nullptr;
};

} // namespace fire_engine
