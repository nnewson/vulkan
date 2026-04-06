#pragma once

#include <memory>
#include <vector>

#include <fire_engine/input/camera_state.hpp>
#include <fire_engine/math/mat4.hpp>
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

    void update(const CameraState& input_state);
    void render();

private:
    std::vector<std::unique_ptr<Node>> nodes_;
    Mat4 rootTransform_{Mat4::identity()};
};

} // namespace fire_engine
