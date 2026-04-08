#include <cstdlib>
#include <cstring>
#include <memory>

#include <fire_engine/fire_engine.hpp>

#include <fire_engine/core/system.hpp>
#include <fire_engine/scene/animator.hpp>
#include <fire_engine/scene/camera.hpp>
#include <fire_engine/scene/mesh.hpp>

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

    loadScene();
    mainLoop();
}

void FireEngine::loadScene()
{
    // Camera
    auto cameraNode = std::make_unique<Node>("Camera");
    auto& camera = cameraNode->component().emplace<Camera>();
    camera.localPosition({2.0f, 2.0f, 2.0f});
    camera.localPitch(-0.615f);
    camera.localYaw(-2.356f);
    auto& camRef = scene_.addNode(std::move(cameraNode));
    scene_.activeCamera(&camRef);

    // Animator with animated capsule mesh as child
    auto animNode = std::make_unique<Node>("Animator");
    animNode->component().emplace<Animator>();
    auto& animRef = scene_.addNode(std::move(animNode));

    auto capsuleNode = std::make_unique<Node>("Capsule");
    capsuleNode->component().emplace<Mesh>();
    auto& capsuleRef = animRef.addChild(std::move(capsuleNode));
    auto& capsule = std::get<Mesh>(capsuleRef.component());
    capsule.load("capsule.obj", "capsule.mtl", renderer_->device(), renderer_->pipeline(),
                 renderer_->frame());

    // Static teapot mesh at scene root (no animator parent)
    auto teapotNode = std::make_unique<Node>("Teapot");
    teapotNode->transform().scale({0.03f, 0.03f, 0.03f});
    teapotNode->transform().position({5.0f, 0.0f, 5.0f});
    teapotNode->component().emplace<Mesh>();
    auto& teapotRef = scene_.addNode(std::move(teapotNode));
    auto& teapot = std::get<Mesh>(teapotRef.component());
    teapot.load("teapot.obj", "default.mtl", renderer_->device(), renderer_->pipeline(),
                renderer_->frame());
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

        renderer_->drawFrame(*window_, scene_);
    }
    renderer_->waitIdle();
}

} // namespace fire_engine
