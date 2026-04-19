#include <fire_engine/graphics/texture.hpp>

#include <fire_engine/render/resources.hpp>

namespace fire_engine
{

Texture Texture::load_from_file(const std::string& path, Resources& resources,
                                const SamplerSettings& sampler)
{
    Image img = Image::load_from_file(path);
    Texture tex;
    tex.handle_ = resources.createTexture(img, sampler);
    tex.samplerSettings_ = sampler;
    return tex;
}

Texture Texture::load_from_data(const uint8_t* pixels, int width, int height,
                                Resources& resources, const SamplerSettings& sampler)
{
    Texture tex;
    tex.handle_ = resources.createTexture(pixels, width, height, sampler);
    tex.samplerSettings_ = sampler;
    return tex;
}

} // namespace fire_engine
