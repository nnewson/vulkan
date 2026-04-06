#pragma once

#include <fire_engine/input/camera_state.hpp>
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

    [[nodiscard]] CameraState update(const Window& window, float deltaTime);

    void enable(const Window& window);

private:
    Keyboard keyboard_;
    Mouse mouse_;

    static constexpr float speed_{10.0f};
    static constexpr float sensitivity_{0.003f};
};

} // namespace fire_engine
