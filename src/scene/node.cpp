#include <variant>

#include "fire_engine/input/camera_state.hpp"
#include <fire_engine/scene/node.hpp>

namespace fire_engine
{

Node::Node(std::string name)
    : name_{std::move(name)}
{
}

Node& Node::addChild(std::unique_ptr<Node> child)
{
    child->parent_ = this;
    children_.push_back(std::move(child));
    return *children_.back();
}

void Node::update(const CameraState& input_state, const Mat4& parentWorld)
{
    transform_.update(parentWorld);

    std::visit([&input_state, this](auto& component)
               { component.update(input_state, transform_); }, component_);

    for (auto& child : children_)
    {
        child->update(input_state, transform_.world());
    }
}

} // namespace fire_engine
