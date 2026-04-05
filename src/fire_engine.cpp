#include <cstdlib>
#include <cstring>

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

    mouse_.enable(*window_);

    driver_->init(*window_);
    mainLoop();
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
        pollCamera(dt);

        driver_->drawFrame(*window_, camera_.position(), camera_.target());
    }
    driver_->waitIdle();
}

void FireEngine::pollCamera(float dt)
{
    keyboard_.poll(*window_);

    if (keyboard_.escape())
    {
        mouse_.release(*window_);
    }
    mouse_.capture(*window_);

    float speed = 10.0f;
    Vec3 pos = camera_.position();

    if (keyboard_.w())
        pos.z(pos.z() - speed * dt);
    if (keyboard_.s())
        pos.z(pos.z() + speed * dt);
    if (keyboard_.a())
        pos.x(pos.x() - speed * dt);
    if (keyboard_.d())
        pos.x(pos.x() + speed * dt);
    if (keyboard_.q())
        pos.y(pos.y() + speed * dt);
    if (keyboard_.e())
        pos.y(pos.y() - speed * dt);

    camera_.position(pos);

    mouse_.poll(*window_);
    if (mouse_.captured())
    {
        float sensitivity = 0.003f;
        camera_.yaw(camera_.yaw() + static_cast<float>(mouse_.deltaX()) * sensitivity);
        camera_.pitch(camera_.pitch() - static_cast<float>(mouse_.deltaY()) * sensitivity);
    }
}

void FireEngine::framebufferResized()
{
    driver_->framebufferResized();
}

} // namespace fire_engine
