#include <cstdlib>
#include <cstring>

#include <fire_engine/fire_engine.hpp>

#include <fire_engine/geometry.hpp>

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

    display_ = std::make_unique<Display>(width, height, app_name,
                                         [](GLFWwindow* w, int, int)
                                         {
                                             auto* engine = static_cast<FireEngine*>(
                                                 glfwGetWindowUserPointer(w));
                                             engine->framebufferResized();
                                         });

    driver_->init(*display_);
    mainLoop();
}

void FireEngine::mainLoop()
{
    while (!glfwWindowShouldClose(display_->getWindow()))
    {
        glfwPollEvents();
        driver_->drawFrame(*display_);
    }
    driver_->waitIdle();
}

void FireEngine::framebufferResized()
{
    driver_->framebufferResized();
}

} // namespace fire_engine
