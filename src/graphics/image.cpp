#include <fire_engine/graphics/image.hpp>

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace fire_engine
{

Image Image::load_from_file(const std::string& path)
{
    Image img;
    int fileChannels = 0;
    constexpr int desiredChannels = 4; // Always load as RGBA

    stbi_uc* raw =
        stbi_load(path.c_str(), &img.width_, &img.height_, &fileChannels, desiredChannels);
    if (!raw)
    {
        throw std::runtime_error("Failed to load image: " + path);
    }

    img.channels_ = desiredChannels;
    std::size_t byteCount = static_cast<std::size_t>(img.width_) * img.height_ * desiredChannels;
    img.pixels_.assign(raw, raw + byteCount);

    stbi_image_free(raw);

    return img;
}

Image::Image(Image&& other) noexcept
    : width_(other.width_),
      height_(other.height_),
      channels_(other.channels_),
      pixels_(std::move(other.pixels_))
{
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        pixels_ = std::move(other.pixels_);
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }
    return *this;
}

} // namespace fire_engine
