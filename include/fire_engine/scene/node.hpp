#pragma once

#include <memory>
#include <string>
#include <vector>

#include <fire_engine/input/camera_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/renderer/render_context.hpp>
#include <fire_engine/scene/components.hpp>
#include <fire_engine/scene/transform.hpp>

namespace fire_engine
{

class Node
{
public:
    Node() = default;
    explicit Node(std::string name);
    ~Node() = default;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    [[nodiscard]] const std::string& name() const noexcept
    {
        return name_;
    }
    void name(std::string n) noexcept
    {
        name_ = std::move(n);
    }

    [[nodiscard]] Transform& transform() noexcept
    {
        return transform_;
    }
    [[nodiscard]] const Transform& transform() const noexcept
    {
        return transform_;
    }

    [[nodiscard]] Components& component() noexcept
    {
        return component_;
    }
    [[nodiscard]] const Components& component() const noexcept
    {
        return component_;
    }

    [[nodiscard]] Node* parent() const noexcept
    {
        return parent_;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const noexcept
    {
        return children_;
    }

    Node& addChild(std::unique_ptr<Node> child);

    void update(const CameraState& input_state, const Mat4& parentWorld);
    void render(const RenderContext& ctx, const Mat4& parentWorld);

private:
    std::string name_;
    Transform transform_;
    Components component_;
    Node* parent_{nullptr};
    std::vector<std::unique_ptr<Node>> children_;
};

} // namespace fire_engine
