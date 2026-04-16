#include <cstdlib>
#include <cstring>
#include <memory>
#include <print>

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

void FireEngine::run(size_t width, size_t height, std::string_view app_name,
                     std::string_view scene_path)
{
    window_ = std::make_unique<Window>(width, height, app_name);
    input_.enable(*window_);

    renderer_ = std::make_unique<Renderer>(*window_);

    loadScene(scene_path);
    mainLoop();
}

void FireEngine::loadScene(std::string_view scene_path)
{
    // Camera
    auto cameraNode = std::make_unique<Node>("Camera");
    auto& camera = cameraNode->component().emplace<Camera>();
    camera.localPosition({2.0f, 2.0f, 2.0f});
    camera.localPitch(-0.615f);
    camera.localYaw(-2.356f);
    scene_.addNode(std::move(cameraNode));
    camera_ = &camera;

    // Load glTF scene (CLI arg overrides default)
    constexpr std::string_view default_scene = "RiggedSimple/RiggedSimple.gltf";
    std::string_view path = scene_path.empty() ? default_scene : scene_path;
    GltfLoader::loadScene(std::string(path), scene_, renderer_->resources(), assets_);

    std::print("{}\n", scene_);
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
