#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fire_engine
{

enum class ImagePixelType : uint8_t
{
    Uint8,
    Float32,
};

class Image
{
public:
    static Image load_from_file(const std::string& path);
    static Image load_from_memory(const uint8_t* data, std::size_t size_bytes,
                                  const std::string& label = "memory");

    Image() = default;
    ~Image() = default;

    Image(const Image&) = default;
    Image& operator=(const Image&) = default;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    [[nodiscard]]
    int width() const noexcept
    {
        return width_;
    }

    [[nodiscard]]
    int height() const noexcept
    {
        return height_;
    }

    [[nodiscard]]
    int channels() const noexcept
    {
        return channels_;
    }

    [[nodiscard]]
    const uint8_t* data() const noexcept
    {
        return pixels8_.empty() ? nullptr : pixels8_.data();
    }

    [[nodiscard]]
    const float* dataf() const noexcept
    {
        return pixels32f_.empty() ? nullptr : pixels32f_.data();
    }

    [[nodiscard]]
    ImagePixelType pixelType() const noexcept
    {
        return pixelType_;
    }

    [[nodiscard]]
    std::size_t size_bytes() const noexcept
    {
        if (pixelType_ == ImagePixelType::Float32)
        {
            return pixels32f_.size() * sizeof(float);
        }

        return pixels8_.size();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return pixels8_.empty() && pixels32f_.empty();
    }

private:
    int width_{0};
    int height_{0};
    int channels_{0};
    ImagePixelType pixelType_{ImagePixelType::Uint8};
    std::vector<uint8_t> pixels8_;
    std::vector<float> pixels32f_;
};

} // namespace fire_engine
