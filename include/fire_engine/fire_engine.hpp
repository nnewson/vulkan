#pragma once

#include "fire_engine/scene/scene_graph.hpp"
#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <memory>

#include <fire_engine/graphics_driver.hpp>
#include <fire_engine/input/input.hpp>
#include <fire_engine/platform/window.hpp>
#include <fire_engine/scene/camera.hpp>
#include <fire_engine/scene/scene_graph.hpp>

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
    Input input_;
    SceneGraph scene_;

    void loadScene();
    void mainLoop();
    void framebufferResized();
};

} // namespace fire_engine
