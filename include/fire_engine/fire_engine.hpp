#pragma once

#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <memory>

#include <fire_engine/display.hpp>
#include <fire_engine/graphics_driver.hpp>
#include <fire_engine/math.hpp>

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

    void mainLoop();
    void framebufferResized();
};

} // namespace fire_engine
