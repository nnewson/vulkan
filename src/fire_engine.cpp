#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <print>
#include <variant>

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
                     std::string_view scene_path, std::string_view skybox_path)
{
    window_ = std::make_unique<Window>(width, height, app_name);
    input_.enable(*window_);

    renderer_ = std::make_unique<Renderer>(*window_, std::string(skybox_path));

    loadScene(scene_path);
    mainLoop();
}

void FireEngine::loadScene(std::string_view scene_path)
{
    // Load glTF scene (CLI arg overrides default)
    constexpr std::string_view default_scene = "RiggedSimple/RiggedSimple.gltf";
    std::string_view path = scene_path.empty() ? default_scene : scene_path;
    Node* activeCamera =
        GltfLoader::loadScene(std::string(path), scene_, renderer_->resources(), assets_, physics_);

    if (activeCamera != nullptr)
    {
        camera_ = std::get_if<Camera>(&activeCamera->component());
    }

    if (camera_ == nullptr)
    {
        auto cameraNode = std::make_unique<Node>("Camera");
        auto& camera = cameraNode->component().emplace<Camera>();
        camera.localPosition({2.0f, 2.0f, 2.0f});
        camera.localPitch(-0.615f);
        camera.localYaw(-2.356f);
        scene_.addNode(std::move(cameraNode));
        camera_ = &camera;
    }

    // Seed default directional only when the asset didn't author its own
    // (KHR_lights_punctual). Aim local -Z along (1, -1, 1).normalise() so
    // the surface-to-light vector matches the previously-hardcoded sun.
    if (!scene_.hasDirectionalLight())
    {
        auto sunNode = std::make_unique<Node>("Sun");
        auto& sun = sunNode->component().emplace<Light>();
        sun.type(Light::Type::Directional);
        sun.colour(Colour3{1.0f, 1.0f, 1.0f});
        sun.intensity(directionalLightIntensity);
        Vec3 sunForward = Vec3::normalise(Vec3{1.0f, -1.0f, 1.0f});
        Vec3 baseDir{0.0f, 0.0f, -1.0f};
        Vec3 axis = Vec3::crossProduct(baseDir, sunForward);
        float axisLen = axis.magnitude();
        if (axisLen > float_epsilon)
        {
            axis = Vec3{axis.x() / axisLen, axis.y() / axisLen, axis.z() / axisLen};
            float angle = std::acos(std::clamp(Vec3::dotProduct(baseDir, sunForward), -1.0f, 1.0f));
            float h = angle * 0.5f;
            float s = std::sin(h);
            sunNode->transform().rotation(
                Quaternion{axis.x() * s, axis.y() * s, axis.z() * s, std::cos(h)});
        }
        scene_.addNode(std::move(sunNode));
    }

    std::print("{}\n", scene_);
}

void FireEngine::mainLoop()
{
    constexpr float fixedDt = 1.0f / 60.0f;
    constexpr float maxFrameTime = 0.25f;
    double lastTime = System::getTime();
    float accumulator = 0.0f;
    while (!window_->shouldClose())
    {
        double now = System::getTime();
        float dt = std::min(static_cast<float>(now - lastTime), maxFrameTime);
        lastTime = now;

        auto input_state = input_.update(*window_, dt);
        input_state.time(now);
        scene_.update(input_state);
        scene_.submitPhysics(physics_);

        accumulator += dt;
        while (accumulator >= fixedDt)
        {
            physics_.step(fixedDt);
            accumulator -= fixedDt;
        }

        scene_.applyPhysics(physics_);

        renderer_->drawFrame(*window_, scene_, camera_->worldPosition(), camera_->worldTarget());
    }
    renderer_->waitIdle();
}

} // namespace fire_engine
