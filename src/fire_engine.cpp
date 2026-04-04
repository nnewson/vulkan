#include <cmath>
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

    glfwSetInputMode(display_->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    driver_->init(*display_);
    mainLoop();
}

void FireEngine::mainLoop()
{
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(display_->getWindow()))
    {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        glfwPollEvents();
        pollCamera(dt);

        Vec3 target = {
            cameraPos_.x_ + std::cos(cameraPitch_) * std::cos(cameraYaw_),
            cameraPos_.y_ + std::sin(cameraPitch_),
            cameraPos_.z_ + std::cos(cameraPitch_) * std::sin(cameraYaw_),
        };
        driver_->drawFrame(*display_, cameraPos_, target);
    }
    driver_->waitIdle();
}

void FireEngine::pollCamera(float dt)
{
    GLFWwindow* w = display_->getWindow();

    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured_ = false;
    }
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mouseCaptured_)
    {
        glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured_ = true;
        firstMouse_ = true;
    }

    float speed = 10.0f;

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos_.z_ -= speed * dt;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos_.z_ += speed * dt;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos_.x_ -= speed * dt;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos_.x_ += speed * dt;
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos_.y_ += speed * dt;
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos_.y_ -= speed * dt;

    double mouseX, mouseY;
    glfwGetCursorPos(w, &mouseX, &mouseY);
    if (firstMouse_)
    {
        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;
        firstMouse_ = false;
    }
    if (mouseCaptured_)
    {
        float sensitivity = 0.003f;
        cameraYaw_ += static_cast<float>(mouseX - lastMouseX_) * sensitivity;
        cameraPitch_ -= static_cast<float>(mouseY - lastMouseY_) * sensitivity;
    }
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;

    constexpr float maxPitch = 1.5f;
    if (cameraPitch_ > maxPitch)
        cameraPitch_ = maxPitch;
    if (cameraPitch_ < -maxPitch)
        cameraPitch_ = -maxPitch;
}

void FireEngine::framebufferResized()
{
    driver_->framebufferResized();
}

} // namespace fire_engine
