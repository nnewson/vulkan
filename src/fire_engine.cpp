#include <cstdlib>
#include <cstring>
#include <memory>

#include <fire_engine/fire_engine.hpp>

#include <fire_engine/core/gltf_loader.hpp>
#include <fire_engine/core/system.hpp>

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
    scene_.addNode(std::move(cameraNode));
    camera_ = &camera;

    // Load glTF scene
    GltfLoader::loadScene("AnimatedCube/AnimatedCube.gltf", scene_, *renderer_);
    // GltfLoader::loadScene("BoxAnimated/BoxAnimated.gltf", scene_, *renderer_);
}

void FireEngine::mainLoop()
{
    double lastTime = System::getTime();
    while (!window_->shouldClose())
    {
        double now = System::getTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        auto input_state = input_.update(*window_, dt);
        input_state.time(now);
        scene_.update(input_state);

        renderer_->drawFrame(*window_, scene_, camera_->worldPosition(), camera_->worldTarget());
    }
    renderer_->waitIdle();
}

} // namespace fire_engine
