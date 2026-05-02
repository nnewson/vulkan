#pragma once

#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <fire_engine/collision/collider.hpp>
#include <fire_engine/input/input_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/render/render_context.hpp>
#include <fire_engine/scene/components.hpp>
#include <fire_engine/scene/controllable.hpp>
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

    [[nodiscard]] bool hasCollider() const noexcept
    {
        return collider_.has_value();
    }
    [[nodiscard]] Collider* collider() noexcept
    {
        return collider_ ? &collider_.value() : nullptr;
    }
    [[nodiscard]] const Collider* collider() const noexcept
    {
        return collider_ ? &collider_.value() : nullptr;
    }
    Collider& emplaceCollider()
    {
        return collider_.emplace();
    }

    [[nodiscard]] bool hasControllable() const noexcept
    {
        return controllable_.has_value();
    }
    [[nodiscard]] Controllable* controllable() noexcept
    {
        return controllable_ ? &controllable_.value() : nullptr;
    }
    [[nodiscard]] const Controllable* controllable() const noexcept
    {
        return controllable_ ? &controllable_.value() : nullptr;
    }
    Controllable& emplaceControllable()
    {
        return controllable_.emplace();
    }

    [[nodiscard]] Node* parent() const noexcept
    {
        return parent_;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const noexcept
    {
        return children_;
    }

    [[nodiscard]] const Mat4& composedWorld() const noexcept
    {
        return composedWorld_;
    }

    Node& addChild(std::unique_ptr<Node> child);

    void update(const InputState& input_state, const Mat4& parentComposedWorld);
    void render(const RenderContext& ctx, const Mat4& parentWorld);

private:
    std::string name_;
    Transform transform_;
    Components component_;
    std::optional<Collider> collider_;
    std::optional<Controllable> controllable_;
    Mat4 composedWorld_{Mat4::identity()};
    Node* parent_{nullptr};
    std::vector<std::unique_ptr<Node>> children_;
};

} // namespace fire_engine

namespace fire_engine::detail
{

inline auto formatNode(const fire_engine::Node& node, std::format_context& ctx, int depth)
    -> std::format_context::iterator
{
    auto indent = std::string(static_cast<std::size_t>(depth * 2), ' ');

    ctx.advance_to(std::format_to(ctx.out(), "{}{} [{}] {}", indent, node.name(),
                                  fire_engine::componentName(node.component()), node.transform()));

    for (const auto& child : node.children())
    {
        ctx.advance_to(std::format_to(ctx.out(), "\n"));
        ctx.advance_to(formatNode(*child, ctx, depth + 1));
    }

    return ctx.out();
}

} // namespace fire_engine::detail

template <>
struct std::formatter<fire_engine::Node>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const fire_engine::Node& node, std::format_context& ctx) const
    {
        return fire_engine::detail::formatNode(node, ctx, 0);
    }
};
