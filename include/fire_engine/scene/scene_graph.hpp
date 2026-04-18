#pragma once

#include <format>
#include <memory>
#include <vector>

#include <fire_engine/input/input_state.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/render/render_context.hpp>
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

    void update(const InputState& input_state);
    void render(const RenderContext& ctx);

private:
    std::vector<std::unique_ptr<Node>> nodes_;
    Mat4 rootTransform_{Mat4::identity()};
};

} // namespace fire_engine

template <>
struct std::formatter<fire_engine::SceneGraph>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const fire_engine::SceneGraph& scene, std::format_context& ctx) const
    {
        ctx.advance_to(std::format_to(ctx.out(), "SceneGraph:"));
        for (const auto& node : scene.nodes())
        {
            ctx.advance_to(std::format_to(ctx.out(), "\n"));
            ctx.advance_to(fire_engine::detail::formatNode(*node, ctx, 1));
        }
        return ctx.out();
    }
};
