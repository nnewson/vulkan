#pragma once

#include <fire_engine/input/input_state.hpp>
#include <fire_engine/platform/keyboard.hpp>
#include <fire_engine/platform/mouse.hpp>
#include <fire_engine/platform/window.hpp>

namespace fire_engine
{

class Input
{
public:
    Input() = default;
    ~Input() = default;

    Input(const Input&) = default;
    Input& operator=(const Input&) = default;
    Input(Input&&) noexcept = default;
    Input& operator=(Input&&) noexcept = default;

    [[nodiscard]] InputState update(const Window& window, float deltaTime);

    void enable(const Window& window);

private:
    Keyboard keyboard_;
    Mouse mouse_;

    static constexpr float speed_{10.0f};
    static constexpr float sensitivity_{0.003f};
    static constexpr float panSensitivity_{0.01f};
    static constexpr float zoomSpeed_{1.0f};
};

} // namespace fire_engine
