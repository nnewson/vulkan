#include <fire_engine/graphics/texture.hpp>

#include <utility>

#include <fire_engine/graphics/ktx_image.hpp>
#include <fire_engine/render/resources.hpp>

namespace fire_engine
{

Texture Texture::load_from_image(const Image& image, Resources& resources,
                                 const SamplerSettings& sampler, TextureEncoding encoding)
{
    Texture tex;
    tex.handle_ = resources.createTexture(image, sampler, encoding);
    tex.samplerSettings_ = sampler;
    tex.encoding_ = encoding;
    return tex;
}

Texture Texture::load_from_file(const std::string& path, Resources& resources,
                                const SamplerSettings& sampler, TextureEncoding encoding)
{
    Image img = Image::load_from_file(path);
    return load_from_image(img, resources, sampler, encoding);
}

Texture Texture::load_from_ktx_image(KtxImage image, Resources& resources,
                                     const SamplerSettings& sampler, TextureEncoding encoding)
{
    Texture tex;
    tex.handle_ = resources.createTexture(std::move(image), sampler, encoding);
    tex.samplerSettings_ = sampler;
    tex.encoding_ = encoding;
    return tex;
}

Texture Texture::load_from_ktx_file(const std::string& path, Resources& resources,
                                    const SamplerSettings& sampler, TextureEncoding encoding)
{
    KtxImage image = KtxImage::load_from_file(path);
    return load_from_ktx_image(std::move(image), resources, sampler, encoding);
}

Texture Texture::load_from_data(const uint8_t* pixels, int width, int height, Resources& resources,
                                const SamplerSettings& sampler, TextureEncoding encoding)
{
    Texture tex;
    tex.handle_ = resources.createTexture(pixels, width, height, sampler, encoding);
    tex.samplerSettings_ = sampler;
    tex.encoding_ = encoding;
    return tex;
}

} // namespace fire_engine
