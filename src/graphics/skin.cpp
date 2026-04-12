#include <fire_engine/graphics/skin.hpp>

#include <fire_engine/scene/node.hpp>

namespace fire_engine
{

void Skin::addJoint(const Node* node, const Mat4& inverseBindMatrix)
{
    jointNodes_.push_back(node);
    inverseBindMatrices_.push_back(inverseBindMatrix);
}

void Skin::updateJointMatrices()
{
    cachedJointMatrices_ = computeJointMatrices();
}

std::vector<Mat4> Skin::computeJointMatrices() const
{
    std::vector<Mat4> matrices(jointNodes_.size());
    for (std::size_t i = 0; i < jointNodes_.size(); ++i)
    {
        matrices[i] = jointNodes_[i]->composedWorld() * inverseBindMatrices_[i];
    }
    return matrices;
}

} // namespace fire_engine
