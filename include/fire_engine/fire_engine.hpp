#pragma once

#include <cstddef>
#include <memory>

#include <fire_engine/graphics/assets.hpp>
#include <fire_engine/input/input.hpp>
#include <fire_engine/platform/window.hpp>
#include <fire_engine/renderer/renderer.hpp>
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
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer> renderer_;
    Input input_;
    SceneGraph scene_;
    Assets assets_;
    Camera* camera_{nullptr};

    void loadScene();
    void mainLoop();
};

} // namespace fire_engine
