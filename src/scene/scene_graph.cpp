#include <variant>

#include <fire_engine/scene/light.hpp>
#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

namespace
{
void gatherLightsRecursive(const Node& node, std::vector<Lighting>& out)
{
    if (auto* light = std::get_if<Light>(&node.component()))
    {
        out.push_back(Light::toLighting(*light, node.composedWorld()));
    }
    for (const auto& child : node.children())
    {
        gatherLightsRecursive(*child, out);
    }
}

bool hasDirectionalLightRecursive(const Node& node)
{
    if (auto* light = std::get_if<Light>(&node.component()))
    {
        if (light->type() == Light::Type::Directional)
        {
            return true;
        }
    }
    for (const auto& child : node.children())
    {
        if (hasDirectionalLightRecursive(*child))
        {
            return true;
        }
    }
    return false;
}
} // namespace

Node& SceneGraph::addNode(std::unique_ptr<Node> node)
{
    nodes_.push_back(std::move(node));
    return *nodes_.back();
}

void SceneGraph::update(const InputState& input_state)
{
    for (auto& node : nodes_)
    {
        node->update(input_state, rootTransform_);
    }
}

void SceneGraph::resolve()
{
    for (auto& node : nodes_)
    {
        node->resolve(rootTransform_);
    }
}

void SceneGraph::render(const RenderContext& ctx)
{
    for (auto& node : nodes_)
    {
        node->render(ctx, rootTransform_);
    }
}

std::vector<Lighting> SceneGraph::gatherLights() const
{
    std::vector<Lighting> out;
    for (const auto& node : nodes_)
    {
        gatherLightsRecursive(*node, out);
    }
    return out;
}

bool SceneGraph::hasDirectionalLight() const
{
    for (const auto& node : nodes_)
    {
        if (hasDirectionalLightRecursive(*node))
        {
            return true;
        }
    }
    return false;
}

} // namespace fire_engine
