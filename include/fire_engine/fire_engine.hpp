#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <memory>

#include <fire_engine/graphics_driver.hpp>
#include <fire_engine/platform/keyboard.hpp>
#include <fire_engine/platform/mouse.hpp>
#include <fire_engine/platform/window.hpp>
#include <fire_engine/scene/camera.hpp>

namespace fire_engine
{

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------
class FireEngine
{
public:
    explicit FireEngine();
    ~FireEngine();

    void run(size_t width, size_t height, std::string_view app_name);

private:
    std::unique_ptr<GraphicsDriver> driver_;
    std::unique_ptr<Window> window_;
    Camera camera_;
    Keyboard keyboard_;
    Mouse mouse_;

    void mainLoop();
    void pollCamera(float dt);
    void framebufferResized();
};

} // namespace fire_engine
