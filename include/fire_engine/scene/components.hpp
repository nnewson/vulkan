#pragma once

#include <string_view>
#include <variant>

#include <fire_engine/scene/animator.hpp>
#include <fire_engine/scene/camera.hpp>
#include <fire_engine/scene/empty.hpp>
#include <fire_engine/scene/light.hpp>
#include <fire_engine/scene/mesh.hpp>

namespace fire_engine
{

using Components = std::variant<Empty, Animator, Camera, Mesh, Light>;

[[nodiscard]] inline std::string_view componentName(const Components& component) noexcept
{
    return std::visit(
        [](const auto& c) -> std::string_view
        {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, Empty>)
            {
                return "Empty";
            }
            else if constexpr (std::is_same_v<T, Animator>)
            {
                return "Animator";
            }
            else if constexpr (std::is_same_v<T, Camera>)
            {
                return "Camera";
            }
            else if constexpr (std::is_same_v<T, Mesh>)
            {
                return "Mesh";
            }
            else if constexpr (std::is_same_v<T, Light>)
            {
                return "Light";
            }
        },
        component);
}

} // namespace fire_engine
