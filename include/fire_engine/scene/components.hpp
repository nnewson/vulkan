#pragma once

#include <variant>

#include <fire_engine/scene/animator.hpp>
#include <fire_engine/scene/camera.hpp>
#include <fire_engine/scene/empty.hpp>
#include <fire_engine/scene/mesh.hpp>

namespace fire_engine
{

using Components = std::variant<Empty, Animator, Camera, Mesh>;

} // namespace fire_engine
