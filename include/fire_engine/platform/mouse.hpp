#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fire_engine/platform/window.hpp>

namespace fire_engine
{

class Mouse
{
public:
    Mouse() = default;
    ~Mouse() = default;

    Mouse(const Mouse&) = default;
    Mouse& operator=(const Mouse&) = default;
    Mouse(Mouse&&) noexcept = default;
    Mouse& operator=(Mouse&&) noexcept = default;

    void poll(const Window& window)
    {
        GLFWwindow* w = window.getWindow();
        double currentX, currentY;
        glfwGetCursorPos(w, &currentX, &currentY);

        if (firstMouse_)
        {
            lastX_ = currentX;
            lastY_ = currentY;
            firstMouse_ = false;
        }

        deltaX_ = currentX - lastX_;
        deltaY_ = currentY - lastY_;

        lastX_ = currentX;
        lastY_ = currentY;
    }

    void capture(const Window& window)
    {
        if (captured_)
        {
            return;
        }
        if (glfwGetMouseButton(window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        {
            return;
        }

        enable(window);
    }

    void release(const Window& window)
    {
        glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        captured_ = false;
    }

    void enable(const Window& window)
    {
        glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        captured_ = true;
        firstMouse_ = true;
    }

    [[nodiscard]] double x() const noexcept
    {
        return lastX_;
    }
    [[nodiscard]] double y() const noexcept
    {
        return lastY_;
    }
    [[nodiscard]] double deltaX() const noexcept
    {
        return deltaX_;
    }
    [[nodiscard]] double deltaY() const noexcept
    {
        return deltaY_;
    }
    [[nodiscard]] bool captured() const noexcept
    {
        return captured_;
    }

private:
    double lastX_{0.0};
    double lastY_{0.0};
    double deltaX_{0.0};
    double deltaY_{0.0};
    bool firstMouse_{true};
    bool captured_{true};
};

} // namespace fire_engine
