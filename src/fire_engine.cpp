#include "fire_engine/scene/camera.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>

#include <fire_engine/fire_engine.hpp>

#include <fire_engine/core/system.hpp>
#include <fire_engine/graphics/geometry.hpp>

namespace fire_engine
{

// ---------------------------------------------------------------------------
// FireEngine
// ---------------------------------------------------------------------------

FireEngine::FireEngine()
{
    System::init();
}

FireEngine::~FireEngine()
{
    System::destroy();
}

void FireEngine::run(size_t width, size_t height, std::string_view app_name)
{
    window_ = std::make_unique<Window>(width, height, app_name);
    input_.enable(*window_);

    renderer_ = std::make_unique<Renderer>(*window_);
    driver_ = std::make_unique<GraphicsDriver>(*renderer_);
    driver_->init();

    loadScene();
    mainLoop();
}

void FireEngine::loadScene()
{
    auto cameraNode = std::make_unique<Node>("Camera");
    auto camera = cameraNode->component().emplace<Camera>();

    camera.localPosition({2.0f, 2.0f, 2.0f});
    camera.localPitch(-0.615f);
    camera.localYaw(-2.356f);

    scene_.addNode(std::move(cameraNode));
}

void FireEngine::mainLoop()
{
    double lastTime = System::getTime();
    while (!window_->shouldClose())
    {
        double now = System::getTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        glfwPollEvents();
        auto input_state = input_.update(*window_, dt);
        scene_.update(input_state);

        scene_.render();
        // DIRTY HACK - this will soon be gone when we move rendering into the scene graph
        auto camera = get<Camera>((*scene_.nodes()[0]).component());

        driver_->drawFrame(*window_, camera.worldPosition(), camera.worldTarget());
    }
    driver_->waitIdle();
}

} // namespace fire_engine
