#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fire_engine/platform/window.hpp>

namespace fire_engine
{

class Keyboard
{
public:
    Keyboard() = default;
    ~Keyboard() = default;

    Keyboard(const Keyboard&) = default;
    Keyboard& operator=(const Keyboard&) = default;
    Keyboard(Keyboard&&) noexcept = default;
    Keyboard& operator=(Keyboard&&) noexcept = default;

    void poll(const Window& window)
    {
        GLFWwindow* w = window.getWindow();
        escape_ = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        w_ = glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
        s_ = glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
        a_ = glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        d_ = glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        q_ = glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS;
        e_ = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
    }

    [[nodiscard]] bool escape() const noexcept { return escape_; }
    [[nodiscard]] bool w() const noexcept { return w_; }
    [[nodiscard]] bool s() const noexcept { return s_; }
    [[nodiscard]] bool a() const noexcept { return a_; }
    [[nodiscard]] bool d() const noexcept { return d_; }
    [[nodiscard]] bool q() const noexcept { return q_; }
    [[nodiscard]] bool e() const noexcept { return e_; }

private:
    bool escape_{false};
    bool w_{false};
    bool s_{false};
    bool a_{false};
    bool d_{false};
    bool q_{false};
    bool e_{false};
};

} // namespace fire_engine
