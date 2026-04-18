#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

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

void SceneGraph::render(const RenderContext& ctx)
{
    for (auto& node : nodes_)
    {
        node->render(ctx, rootTransform_);
    }
}

} // namespace fire_engine
