#pragma once

#include <string>

#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/graphics/image.hpp>
#include <fire_engine/graphics/sampler_settings.hpp>

namespace fire_engine
{

class Resources;
class KtxImage;

enum class TextureEncoding : uint8_t
{
    Srgb,
    Linear,
};

class Texture
{
public:
    static Texture load_from_image(const Image& image, Resources& resources,
                                   const SamplerSettings& sampler = {},
                                   TextureEncoding encoding = TextureEncoding::Srgb);

    static Texture load_from_file(const std::string& path, Resources& resources,
                                  const SamplerSettings& sampler = {},
                                  TextureEncoding encoding = TextureEncoding::Srgb);

    static Texture load_from_ktx_image(KtxImage image, Resources& resources,
                                       const SamplerSettings& sampler = {},
                                       TextureEncoding encoding = TextureEncoding::Srgb);

    static Texture load_from_ktx_file(const std::string& path, Resources& resources,
                                      const SamplerSettings& sampler = {},
                                      TextureEncoding encoding = TextureEncoding::Srgb);

    static Texture load_from_data(const uint8_t* pixels, int width, int height,
                                  Resources& resources, const SamplerSettings& sampler = {},
                                  TextureEncoding encoding = TextureEncoding::Srgb);

    Texture() = default;
    ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;

    [[nodiscard]] TextureHandle handle() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] bool loaded() const noexcept
    {
        return handle_ != NullTexture;
    }

    [[nodiscard]] const SamplerSettings& samplerSettings() const noexcept
    {
        return samplerSettings_;
    }

    [[nodiscard]] TextureEncoding encoding() const noexcept
    {
        return encoding_;
    }

private:
    TextureHandle handle_{NullTexture};
    SamplerSettings samplerSettings_{};
    TextureEncoding encoding_{TextureEncoding::Srgb};
};

} // namespace fire_engine
