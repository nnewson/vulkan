#include <variant>

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

void Node::update(const InputState& input_state, const Mat4& parentComposedWorld)
{
    frameStartPosition_ = transform_.position();

    if (controllable_)
    {
        controllable_->update(input_state.controllerState(), transform_, parentComposedWorld);
    }
    else if (physicsBody_)
    {
        transform_.position(transform_.position() +
                            physicsBody_->velocity() * input_state.deltaTime());
        transform_.update(parentComposedWorld);
    }
    else
    {
        transform_.update(parentComposedWorld);
    }
    frameDelta_ = transform_.position() - frameStartPosition_;

    std::visit([&input_state, this](auto& component) { component.update(input_state, transform_); },
               component_);

    // Composed world includes component effects (e.g. Animator's model matrix)
    Mat4 componentMatrix = std::visit([](const auto& component) -> Mat4
                                      { return component.modelMatrix(); }, component_);
    composedWorld_ = parentComposedWorld * transform_.local() * componentMatrix;
    if (collider_)
    {
        collider_->update(composedWorld_);
    }

    for (auto& child : children_)
    {
        child->update(input_state, composedWorld_);
    }
}

void Node::moveToFrameTime(float toi) noexcept
{
    transform_.position(frameStartPosition_ + frameDelta_ * toi);
    frameDelta_ = {};
}

void Node::slideFrameMovement(float toi, Vec3 normal) noexcept
{
    const float blocked = Vec3::dotProduct(frameDelta_, normal);
    if (blocked >= 0.0f)
    {
        return;
    }

    const Vec3 tangentDelta = frameDelta_ - normal * blocked;
    const Vec3 resolvedDelta = frameDelta_ * toi + tangentDelta * (1.0f - toi);
    transform_.position(frameStartPosition_ + resolvedDelta);
    frameDelta_ = resolvedDelta;
}

void Node::resolve(const Mat4& parentComposedWorld)
{
    transform_.update(parentComposedWorld);

    Mat4 componentMatrix = std::visit([](const auto& component) -> Mat4
                                      { return component.modelMatrix(); }, component_);
    composedWorld_ = parentComposedWorld * transform_.local() * componentMatrix;
    if (collider_)
    {
        collider_->resetFrame(composedWorld_);
    }

    for (auto& child : children_)
    {
        child->resolve(composedWorld_);
    }
}

void Node::render(const RenderContext& ctx, const Mat4& parentWorld)
{
    Mat4 world = parentWorld * transform_.local();
    Mat4 childWorld = std::visit([&ctx, &world](auto& component) -> Mat4
                                 { return component.render(ctx, world); }, component_);

    for (auto& child : children_)
    {
        child->render(ctx, childWorld);
    }
}

} // namespace fire_engine
