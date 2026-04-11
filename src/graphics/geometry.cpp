#include "fire_engine/render/renderer.hpp"
#include <fire_engine/graphics/geometry.hpp>

#include <fire_engine/render/device.hpp>

namespace fire_engine
{

void Geometry::load(const Renderer& renderer)
{
    auto& device = renderer.device();
    vkDevice_ = &device.device();

    // Create vertex buffer
    vk::DeviceSize vertexBufSize = sizeof(vertices_[0]) * vertices_.size();
    auto [vBuf, vMem] = device.createBuffer(vertexBufSize, vk::BufferUsageFlagBits::eVertexBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    vertexBuf_ = std::move(vBuf);
    vertexMem_ = std::move(vMem);
    void* vertData = vertexMem_.mapMemory(0, vertexBufSize);
    memcpy(vertData, vertices_.data(), vertexBufSize);
    vertexMem_.unmapMemory();

    // Create index buffer
    vk::DeviceSize indexBufSize = sizeof(indices_[0]) * indices_.size();
    auto [iBuf, iMem] = device.createBuffer(indexBufSize, vk::BufferUsageFlagBits::eIndexBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    indexBuf_ = std::move(iBuf);
    indexMem_ = std::move(iMem);
    void* indexData = indexMem_.mapMemory(0, indexBufSize);
    memcpy(indexData, indices_.data(), indexBufSize);
    indexMem_.unmapMemory();
}

} // namespace fire_engine
