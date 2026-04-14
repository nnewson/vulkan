#pragma once

#include <array>
#include <vector>

#include <fire_engine/graphics/draw_command.hpp>
#include <fire_engine/graphics/frame_info.hpp>
#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/math/mat4.hpp>
#include <fire_engine/render/constants.hpp>

namespace fire_engine
{

class Geometry;
class Material;
class Resources;
class Skin;
struct MaterialUBO;

class Object
{
public:
    Object() = default;
    ~Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    Object(Object&&) noexcept = default;
    Object& operator=(Object&&) noexcept = default;

    void addGeometry(const Geometry& geometry);
    void load(Resources& resources);

    void skin(Skin* s) noexcept
    {
        skin_ = s;
    }
    [[nodiscard]] const Skin* skin() const noexcept
    {
        return skin_;
    }

    void updateSkin();

    void morphWeights(const std::vector<float>& weights) noexcept
    {
        morphWeights_ = weights;
    }

    [[nodiscard]]
    std::vector<DrawCommand> render(const FrameInfo& frame, const Mat4& world);

private:
    struct GeometryBindings
    {
        const Geometry* geometry{nullptr};

        std::array<MappedMemory, MAX_FRAMES_IN_FLIGHT> materialMapped{};
        std::array<MappedMemory, MAX_FRAMES_IN_FLIGHT> skinMapped{};
        std::array<MappedMemory, MAX_FRAMES_IN_FLIGHT> morphUboMapped{};
        std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT> descSets{
            NullDescriptorSet, NullDescriptorSet};
    };

    static MaterialUBO toMaterialUBO(const Material& mat);

    Skin* skin_{nullptr};
    std::vector<float> morphWeights_;

    std::array<MappedMemory, MAX_FRAMES_IN_FLIGHT> uniformMapped_{};

    std::vector<GeometryBindings> bindings_;
};

} // namespace fire_engine
