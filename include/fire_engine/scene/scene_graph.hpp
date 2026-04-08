#pragma once

#include <memory>
#include <vector>

#include <fire_engine/input/camera_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/renderer/render_context.hpp>
#include <fire_engine/scene/node.hpp>

namespace fire_engine
{

class SceneGraph
{
public:
    SceneGraph() = default;
    ~SceneGraph() = default;

    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;
    SceneGraph(SceneGraph&&) noexcept = default;
    SceneGraph& operator=(SceneGraph&&) noexcept = default;

    Node& addNode(std::unique_ptr<Node> node);

    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& nodes() const noexcept
    {
        return nodes_;
    }

    [[nodiscard]] Mat4 rootTransform() const noexcept
    {
        return rootTransform_;
    }
    void rootTransform(Mat4 t) noexcept
    {
        rootTransform_ = t;
    }

    [[nodiscard]] Node* activeCamera() const noexcept
    {
        return activeCamera_;
    }
    void activeCamera(Node* node) noexcept
    {
        activeCamera_ = node;
    }

    [[nodiscard]] Vec3 cameraPosition() const noexcept
    {
        return cameraPosition_;
    }

    [[nodiscard]] Vec3 cameraTarget() const noexcept
    {
        return cameraTarget_;
    }

    void update(const CameraState& input_state);
    void render(const RenderContext& ctx);

private:
    std::vector<std::unique_ptr<Node>> nodes_;
    Mat4 rootTransform_{Mat4::identity()};
    Node* activeCamera_{nullptr};
    Vec3 cameraPosition_;
    Vec3 cameraTarget_;
};

} // namespace fire_engine
