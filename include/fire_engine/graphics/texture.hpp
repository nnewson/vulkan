#pragma once

#include <string>

#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/graphics/image.hpp>

namespace fire_engine
{

class Resources;

class Texture
{
public:
    static Texture load_from_file(const std::string& path, Resources& resources);

    static Texture load_from_data(const uint8_t* pixels, int width, int height,
                                  Resources& resources);

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

private:
    TextureHandle handle_{NullTexture};
};

} // namespace fire_engine
