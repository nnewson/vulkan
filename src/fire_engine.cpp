#include "fire_engine/scene/camera.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>

#include <fire_engine/fire_engine.hpp>

#include <fire_engine/graphics/geometry.hpp>

namespace fire_engine
{

// ---------------------------------------------------------------------------
// FireEngine
// ---------------------------------------------------------------------------

FireEngine::FireEngine()
{
    glfwInit();
}

FireEngine::~FireEngine()
{
    glfwTerminate();
}

void FireEngine::run(size_t width, size_t height, std::string_view app_name)
{
    driver_ = std::make_unique<GraphicsDriver>();

    window_ = std::make_unique<Window>(width, height, app_name,
                                       [](GLFWwindow* w, int, int)
                                       {
                                           auto* engine = static_cast<FireEngine*>(
                                               glfwGetWindowUserPointer(w));
                                           engine->framebufferResized();
                                       });

    input_.enable(*window_);

    driver_->init(*window_);
    loadScene();
    mainLoop();
}

void FireEngine::loadScene()
{
    auto cameraNode = std::make_unique<Node>("Camera");
    auto camera = cameraNode->component().emplace<Camera>();

    camera.position({2.0f, 2.0f, 2.0f});
    camera.pitch(-0.615f);
    camera.yaw(-2.356f);

    scene_.addNode(std::move(cameraNode));
}

void FireEngine::mainLoop()
{
    double lastTime = glfwGetTime();
    while (!window_->shouldClose())
    {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        glfwPollEvents();
        auto input_state = input_.update(*window_, dt);
        scene_.update(input_state);

        scene_.render();
        auto camera = get<Camera>((*scene_.nodes()[0]).component());

        driver_->drawFrame(*window_, camera.position(), camera.target());
    }
    driver_->waitIdle();
}

void FireEngine::framebufferResized()
{
    driver_->framebufferResized();
}

} // namespace fire_engine
