#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <ktx.h>

namespace fire_engine
{

class KtxImage
{
public:
    static KtxImage load_from_file(const std::string& path);
    static KtxImage load_from_memory(const uint8_t* data, std::size_t size_bytes,
                                     const std::string& label = "memory");

    KtxImage() = default;
    ~KtxImage();

    KtxImage(const KtxImage&) = delete;
    KtxImage& operator=(const KtxImage&) = delete;
    KtxImage(KtxImage&& other) noexcept;
    KtxImage& operator=(KtxImage&& other) noexcept;

    [[nodiscard]]
    uint32_t width() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->baseWidth;
    }

    [[nodiscard]]
    uint32_t height() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->baseHeight;
    }

    [[nodiscard]]
    uint32_t depth() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->baseDepth;
    }

    [[nodiscard]]
    uint32_t dimensions() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->numDimensions;
    }

    [[nodiscard]]
    uint32_t levels() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->numLevels;
    }

    [[nodiscard]]
    uint32_t layers() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->numLayers;
    }

    [[nodiscard]]
    uint32_t faces() const noexcept
    {
        return texture_ == nullptr ? 0u : texture_->numFaces;
    }

    [[nodiscard]]
    bool array() const noexcept
    {
        return texture_ != nullptr && texture_->isArray != 0;
    }

    [[nodiscard]]
    bool cubemap() const noexcept
    {
        return texture_ != nullptr && texture_->isCubemap != 0;
    }

    [[nodiscard]]
    bool compressed() const noexcept
    {
        return texture_ != nullptr && texture_->isCompressed != 0;
    }

    [[nodiscard]]
    bool isKtx2() const noexcept;

    [[nodiscard]]
    bool needsTranscoding() const noexcept;

    [[nodiscard]]
    uint32_t vkFormat() const noexcept;

    [[nodiscard]]
    uint32_t element_size() const noexcept;

    [[nodiscard]]
    std::size_t size_bytes() const noexcept;

    [[nodiscard]]
    const uint8_t* data() const noexcept;

    [[nodiscard]]
    bool empty() const noexcept
    {
        return texture_ == nullptr || data() == nullptr || size_bytes() == 0u;
    }

    [[nodiscard]]
    ktxTexture* native_handle() const noexcept
    {
        return texture_;
    }

private:
    explicit KtxImage(ktxTexture* texture) noexcept
        : texture_(texture)
    {
    }

    void reset() noexcept;

    ktxTexture* texture_{nullptr};
};

} // namespace fire_engine
