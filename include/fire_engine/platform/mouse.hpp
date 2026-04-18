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

        leftButton_ = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        rightButton_ = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    }

    void registerScrollCallback(const Window& window)
    {
        glfwSetScrollCallback(window.getWindow(), scrollCallback);
    }

    [[nodiscard]] double consumeScrollDelta() noexcept
    {
        double delta = scrollAccumulator_;
        scrollAccumulator_ = 0.0;
        return delta;
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
    [[nodiscard]] bool leftButton() const noexcept
    {
        return leftButton_;
    }
    [[nodiscard]] bool rightButton() const noexcept
    {
        return rightButton_;
    }

private:
    static void scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
    {
        scrollAccumulator_ += yoffset;
    }

    static inline double scrollAccumulator_{0.0};

    double lastX_{0.0};
    double lastY_{0.0};
    double deltaX_{0.0};
    double deltaY_{0.0};
    bool firstMouse_{true};
    bool leftButton_{false};
    bool rightButton_{false};
};

} // namespace fire_engine
