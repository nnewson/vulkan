#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <memory>

#include <fire_engine/display.hpp>
#include <fire_engine/graphics_driver.hpp>

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
    std::unique_ptr<Display> display_;
    Vec3 cameraPos_{2.0f, 2.0f, 2.0f};
    float cameraYaw_ = -2.356f;   // radians, initially facing toward origin
    float cameraPitch_ = -0.615f; // radians
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool firstMouse_ = true;
    bool mouseCaptured_ = true;

    void mainLoop();
    void pollCamera(float dt);
    void framebufferResized();
};

} // namespace fire_engine
