#pragma once

#include <variant>

#include <fire_engine/scene/camera.hpp>

namespace fire_engine
{

using Components = std::variant<Camera>;

} // namespace fire_engine
