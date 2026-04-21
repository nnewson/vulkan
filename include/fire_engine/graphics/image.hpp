#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fire_engine
{

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
        return pixels_.data();
    }

    [[nodiscard]]
    std::size_t size_bytes() const noexcept
    {
        return pixels_.size();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return pixels_.empty();
    }

private:
    int width_{0};
    int height_{0};
    int channels_{0};
    std::vector<uint8_t> pixels_;
};

} // namespace fire_engine
