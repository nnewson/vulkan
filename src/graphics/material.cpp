#include <fire_engine/graphics/material.hpp>

namespace fire_engine
{

void Material::loadTexture(const vk::raii::Device& device,
                           const vk::raii::PhysicalDevice& physDevice, vk::CommandPool cmdPool,
                           const vk::raii::Queue& queue)
{
    std::string texPath = mapKd_.empty() ? "default.png" : mapKd_;
    texture_ = Texture::load_from_file(texPath, device, physDevice, cmdPool, queue);
}

} // namespace fire_engine
