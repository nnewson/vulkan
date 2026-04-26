#pragma once

#include <cstdint>
#include <string>

#include <fire_engine/graphics/colour3.hpp>

namespace fire_engine
{

class Texture;

enum class AlphaMode : uint8_t
{
    Opaque,
    Mask,
    Blend,
};

// KHR_texture_transform per-slot UV transform. Default = identity (offset 0,
// scale 1, rotation 0) means "no transform" — matches the spec's behaviour
// when the extension is absent on a TextureInfo.
struct UvTransform
{
    float offsetX{0.0f};
    float offsetY{0.0f};
    float scaleX{1.0f};
    float scaleY{1.0f};
    float rotation{0.0f};
};

class Material
{
public:
    Material() = default;
    ~Material() = default;

    Material(const Material&) = default;
    Material& operator=(const Material&) = default;
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    [[nodiscard]] const Texture& texture() const noexcept
    {
        return *texture_;
    }
    void texture(const Texture* t) noexcept
    {
        texture_ = t;
    }
    [[nodiscard]] bool hasTexture() const noexcept
    {
        return texture_ != nullptr;
    }

    [[nodiscard]] const Texture& emissiveTexture() const noexcept
    {
        return *emissiveTexture_;
    }
    void emissiveTexture(const Texture* t) noexcept
    {
        emissiveTexture_ = t;
    }
    [[nodiscard]] bool hasEmissiveTexture() const noexcept
    {
        return emissiveTexture_ != nullptr;
    }

    [[nodiscard]] const Texture& normalTexture() const noexcept
    {
        return *normalTexture_;
    }
    void normalTexture(const Texture* t) noexcept
    {
        normalTexture_ = t;
    }
    [[nodiscard]] bool hasNormalTexture() const noexcept
    {
        return normalTexture_ != nullptr;
    }

    [[nodiscard]] const Texture& metallicRoughnessTexture() const noexcept
    {
        return *metallicRoughnessTexture_;
    }
    void metallicRoughnessTexture(const Texture* t) noexcept
    {
        metallicRoughnessTexture_ = t;
    }
    [[nodiscard]] bool hasMetallicRoughnessTexture() const noexcept
    {
        return metallicRoughnessTexture_ != nullptr;
    }

    [[nodiscard]] const Texture& occlusionTexture() const noexcept
    {
        return *occlusionTexture_;
    }
    void occlusionTexture(const Texture* t) noexcept
    {
        occlusionTexture_ = t;
    }
    [[nodiscard]] bool hasOcclusionTexture() const noexcept
    {
        return occlusionTexture_ != nullptr;
    }

    [[nodiscard]] const std::string& name() const noexcept
    {
        return name_;
    }
    void name(const std::string& name)
    {
        name_ = name;
    }

    [[nodiscard]] Colour3 diffuse() const noexcept
    {
        return diffuse_;
    }
    void diffuse(Colour3 c) noexcept
    {
        diffuse_ = c;
    }

    [[nodiscard]] Colour3 emissive() const noexcept
    {
        return emissive_;
    }
    void emissive(Colour3 c) noexcept
    {
        emissive_ = c;
    }

    [[nodiscard]] float roughness() const noexcept
    {
        return roughness_;
    }
    void roughness(float v) noexcept
    {
        roughness_ = v;
    }

    [[nodiscard]] float metallic() const noexcept
    {
        return metallic_;
    }
    void metallic(float v) noexcept
    {
        metallic_ = v;
    }

    [[nodiscard]] float alpha() const noexcept
    {
        return alpha_;
    }
    void alpha(float v) noexcept
    {
        alpha_ = v;
    }

    [[nodiscard]] float normalScale() const noexcept
    {
        return normalScale_;
    }
    void normalScale(float v) noexcept
    {
        normalScale_ = v;
    }

    // glTF: shader output = lerp(colour, colour * sampledAO, strength).
    // Spec default is 1.0 (full occlusion contribution).
    [[nodiscard]] float occlusionStrength() const noexcept
    {
        return occlusionStrength_;
    }
    void occlusionStrength(float v) noexcept
    {
        occlusionStrength_ = v;
    }

    // glTF allows each material texture to point at TEXCOORD_0 or TEXCOORD_1.
    // Defaults are 0 — matches glTF spec when omitted. B3 wires the loader to
    // honour the source asset's per-slot index; for now nothing populates
    // anything other than 0, so behaviour is unchanged.
    [[nodiscard]] int baseColorTexCoord() const noexcept { return baseColorTexCoord_; }
    void baseColorTexCoord(int v) noexcept { baseColorTexCoord_ = v; }
    [[nodiscard]] int emissiveTexCoord() const noexcept { return emissiveTexCoord_; }
    void emissiveTexCoord(int v) noexcept { emissiveTexCoord_ = v; }
    [[nodiscard]] int normalTexCoord() const noexcept { return normalTexCoord_; }
    void normalTexCoord(int v) noexcept { normalTexCoord_ = v; }
    [[nodiscard]] int metallicRoughnessTexCoord() const noexcept { return metallicRoughnessTexCoord_; }
    void metallicRoughnessTexCoord(int v) noexcept { metallicRoughnessTexCoord_ = v; }
    [[nodiscard]] int occlusionTexCoord() const noexcept { return occlusionTexCoord_; }
    void occlusionTexCoord(int v) noexcept { occlusionTexCoord_ = v; }

    // KHR_texture_transform per-slot UV transforms. Identity by default.
    [[nodiscard]] UvTransform baseColorUvTransform() const noexcept { return baseColorUvTransform_; }
    void baseColorUvTransform(UvTransform t) noexcept { baseColorUvTransform_ = t; }
    [[nodiscard]] UvTransform emissiveUvTransform() const noexcept { return emissiveUvTransform_; }
    void emissiveUvTransform(UvTransform t) noexcept { emissiveUvTransform_ = t; }
    [[nodiscard]] UvTransform normalUvTransform() const noexcept { return normalUvTransform_; }
    void normalUvTransform(UvTransform t) noexcept { normalUvTransform_ = t; }
    [[nodiscard]] UvTransform metallicRoughnessUvTransform() const noexcept
    {
        return metallicRoughnessUvTransform_;
    }
    void metallicRoughnessUvTransform(UvTransform t) noexcept
    {
        metallicRoughnessUvTransform_ = t;
    }
    [[nodiscard]] UvTransform occlusionUvTransform() const noexcept { return occlusionUvTransform_; }
    void occlusionUvTransform(UvTransform t) noexcept { occlusionUvTransform_ = t; }

    [[nodiscard]] AlphaMode alphaMode() const noexcept
    {
        return alphaMode_;
    }
    void alphaMode(AlphaMode m) noexcept
    {
        alphaMode_ = m;
    }

    [[nodiscard]] float alphaCutoff() const noexcept
    {
        return alphaCutoff_;
    }
    void alphaCutoff(float v) noexcept
    {
        alphaCutoff_ = v;
    }

    [[nodiscard]] bool doubleSided() const noexcept
    {
        return doubleSided_;
    }
    void doubleSided(bool v) noexcept
    {
        doubleSided_ = v;
    }

private:
    std::string name_;
    Colour3 diffuse_{};
    Colour3 emissive_{};
    float roughness_{0.0f};
    float metallic_{0.0f};
    float alpha_{1.0f};
    float normalScale_{1.0f};
    float occlusionStrength_{1.0f};
    int baseColorTexCoord_{0};
    int emissiveTexCoord_{0};
    int normalTexCoord_{0};
    int metallicRoughnessTexCoord_{0};
    int occlusionTexCoord_{0};
    UvTransform baseColorUvTransform_{};
    UvTransform emissiveUvTransform_{};
    UvTransform normalUvTransform_{};
    UvTransform metallicRoughnessUvTransform_{};
    UvTransform occlusionUvTransform_{};
    AlphaMode alphaMode_{AlphaMode::Opaque};
    float alphaCutoff_{0.5f};
    bool doubleSided_{false};

    const Texture* texture_{nullptr};
    const Texture* emissiveTexture_{nullptr};
    const Texture* normalTexture_{nullptr};
    const Texture* metallicRoughnessTexture_{nullptr};
    const Texture* occlusionTexture_{nullptr};
};

} // namespace fire_engine
