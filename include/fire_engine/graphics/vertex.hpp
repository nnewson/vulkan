#pragma once

#include <cstdint>

#include <fire_engine/graphics/colour3.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Vertex
{
public:
    Vertex() = default;
    ~Vertex() = default;

    Vertex(const Vertex&) = default;
    Vertex& operator=(const Vertex&) = default;
    Vertex(Vertex&&) noexcept = default;
    Vertex& operator=(Vertex&&) noexcept = default;

    Vertex(Vec3 position, Colour3 colour, Vec3 normal, float texU, float texV) noexcept
        : position_(position),
          colour_(colour),
          normal_(normal),
          texCoord_{texU, texV}
    {
    }

    Vertex(Vec3 position, Colour3 colour, Vec3 normal, float texU, float texV, uint32_t j0,
           uint32_t j1, uint32_t j2, uint32_t j3, float w0, float w1, float w2, float w3) noexcept
        : position_(position),
          colour_(colour),
          normal_(normal),
          texCoord_{texU, texV},
          joints_{j0, j1, j2, j3},
          weights_{w0, w1, w2, w3}
    {
    }

    [[nodiscard]] Vec3 position() const noexcept
    {
        return position_;
    }
    void position(Vec3 v) noexcept
    {
        position_ = v;
    }

    [[nodiscard]] Colour3 colour() const noexcept
    {
        return colour_;
    }
    void colour(Colour3 c) noexcept
    {
        colour_ = c;
    }

    [[nodiscard]] Vec3 normal() const noexcept
    {
        return normal_;
    }
    void normal(Vec3 v) noexcept
    {
        normal_ = v;
    }

    [[nodiscard]] float texU() const noexcept
    {
        return texCoord_[0];
    }
    void texU(float u) noexcept
    {
        texCoord_[0] = u;
    }

    [[nodiscard]] float texV() const noexcept
    {
        return texCoord_[1];
    }
    void texV(float v) noexcept
    {
        texCoord_[1] = v;
    }

    [[nodiscard]] const uint32_t* joints() const noexcept
    {
        return joints_;
    }
    void joints(uint32_t j0, uint32_t j1, uint32_t j2, uint32_t j3) noexcept
    {
        joints_[0] = j0;
        joints_[1] = j1;
        joints_[2] = j2;
        joints_[3] = j3;
    }

    [[nodiscard]] const float* weights() const noexcept
    {
        return weights_;
    }
    void weights(float w0, float w1, float w2, float w3) noexcept
    {
        weights_[0] = w0;
        weights_[1] = w1;
        weights_[2] = w2;
        weights_[3] = w3;
    }

    [[nodiscard]]
    bool operator==(const Vertex& other) const noexcept
    {
        return position_ == other.position_ && colour_ == other.colour_ &&
               normal_ == other.normal_ && texCoord_[0] == other.texCoord_[0] &&
               texCoord_[1] == other.texCoord_[1];
    }

    friend class Pipeline;

private:
    Vec3 position_{};
    Colour3 colour_{};
    Vec3 normal_{};
    float texCoord_[2]{0.0f, 0.0f};
    uint32_t joints_[4]{0, 0, 0, 0};
    float weights_[4]{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace fire_engine
