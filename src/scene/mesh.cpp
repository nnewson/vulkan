#include "fire_engine/renderer/renderer.hpp"
#include <fire_engine/scene/mesh.hpp>

#include <fire_engine/math/constants.hpp>
#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/frame.hpp>
#include <fire_engine/renderer/pipeline.hpp>
#include <fire_engine/renderer/render_context.hpp>
#include <fire_engine/renderer/swapchain.hpp>
#include <fire_engine/renderer/ubo.hpp>

namespace fire_engine
{

void Mesh::load(const Geometry& renderData, const Material& material,
                const std::string& texturePath, const Renderer& renderer)
{
    auto& device = renderer.device();
    vkDevice_ = &device.device();

    material_ = material;
    renderData_ = renderData;

    // Create vertex buffer
    vk::DeviceSize vertexBufSize =
        sizeof(renderData_.vertices()[0]) * renderData_.vertices().size();
    auto [vBuf, vMem] = device.createBuffer(vertexBufSize, vk::BufferUsageFlagBits::eVertexBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    vertexBuf_ = std::move(vBuf);
    vertexMem_ = std::move(vMem);
    void* vertData = vertexMem_.mapMemory(0, vertexBufSize);
    memcpy(vertData, renderData_.vertices().data(), vertexBufSize);
    vertexMem_.unmapMemory();

    // Create index buffer
    vk::DeviceSize indexBufSize = sizeof(renderData_.indices()[0]) * renderData_.indices().size();
    auto [iBuf, iMem] = device.createBuffer(indexBufSize, vk::BufferUsageFlagBits::eIndexBuffer,
                                            vk::MemoryPropertyFlagBits::eHostVisible |
                                                vk::MemoryPropertyFlagBits::eHostCoherent);
    indexBuf_ = std::move(iBuf);
    indexMem_ = std::move(iMem);
    void* indexData = indexMem_.mapMemory(0, indexBufSize);
    memcpy(indexData, renderData_.indices().data(), indexBufSize);
    indexMem_.unmapMemory();

    // Load texture
    std::string texPath = texturePath.empty() ? "default.png" : texturePath;
    texture_ = Texture::load_from_file(texPath, *vkDevice_, device.physicalDevice(),
                                       renderer.frame().commandPool(), device.graphicsQueue());

    // Create material uniform buffers
    vk::DeviceSize matSize = sizeof(MaterialUBO);
    materialBufs_.reserve(MAX_FRAMES_IN_FLIGHT);
    materialMems_.reserve(MAX_FRAMES_IN_FLIGHT);
    materialMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [mBuf, mMem] = device.createBuffer(matSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        materialMapped_[i] = mMem.mapMemory(0, matSize);
        materialBufs_.push_back(std::move(mBuf));
        materialMems_.push_back(std::move(mMem));

        MaterialUBO matUbo{};
        matUbo.ambient[0] = material_.ambient().r();
        matUbo.ambient[1] = material_.ambient().g();
        matUbo.ambient[2] = material_.ambient().b();
        matUbo.diffuse[0] = material_.diffuse().r();
        matUbo.diffuse[1] = material_.diffuse().g();
        matUbo.diffuse[2] = material_.diffuse().b();
        matUbo.specular[0] = material_.specular().r();
        matUbo.specular[1] = material_.specular().g();
        matUbo.specular[2] = material_.specular().b();
        matUbo.emissive[0] = material_.emissive().r();
        matUbo.emissive[1] = material_.emissive().g();
        matUbo.emissive[2] = material_.emissive().b();
        matUbo.shininess = material_.shininess();
        matUbo.ior = material_.ior();
        matUbo.transparency = material_.transparency();
        matUbo.illum = material_.illum();
        matUbo.roughness = material_.roughness();
        matUbo.metallic = material_.metallic();
        matUbo.sheen = material_.sheen();
        matUbo.clearcoat = material_.clearcoat();
        matUbo.clearcoatRoughness = material_.clearcoatRoughness();
        matUbo.anisotropy = material_.anisotropy();
        matUbo.anisotropyRotation = material_.anisotropyRotation();
        memcpy(materialMapped_[i], &matUbo, sizeof(matUbo));
    }

    // Create per-mesh UBOs and descriptor sets
    createUniformBuffers(device);
    createDescriptorPool();
    createDescriptorSets(renderer.pipeline());
}

void Mesh::createUniformBuffers(const Device& device)
{
    vk::DeviceSize size = sizeof(UniformBufferObject);
    uniformBufs_.reserve(MAX_FRAMES_IN_FLIGHT);
    uniformMems_.reserve(MAX_FRAMES_IN_FLIGHT);
    uniformMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto [uBuf, uMem] = device.createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
                                                vk::MemoryPropertyFlagBits::eHostVisible |
                                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        uniformMapped_[i] = uMem.mapMemory(0, size);
        uniformBufs_.push_back(std::move(uBuf));
        uniformMems_.push_back(std::move(uMem));
    }
}

void Mesh::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2)},
        {vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
    }};
    vk::DescriptorPoolCreateInfo ci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                    static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), poolSizes);
    descPool_ = vk::raii::DescriptorPool(*vkDevice_, ci);
}

void Mesh::createDescriptorSets(const Pipeline& pipeline)
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 pipeline.descriptorSetLayout());
    vk::DescriptorSetAllocateInfo ai(*descPool_, layouts);
    auto sets = vkDevice_->allocateDescriptorSets(ai);
    descSets_.clear();
    descSets_.reserve(sets.size());
    for (auto& s : sets)
        descSets_.push_back(std::move(s));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo uboBufInfo(*uniformBufs_[i], 0, sizeof(UniformBufferObject));
        vk::DescriptorBufferInfo matBufInfo(*materialBufs_[i], 0, sizeof(MaterialUBO));
        vk::DescriptorImageInfo texInfo(texture_.sampler(), texture_.view(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);

        std::array<vk::WriteDescriptorSet, 3> writes = {{
            {*descSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboBufInfo},
            {*descSets_[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &matBufInfo},
            {*descSets_[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
        }};
        vkDevice_->updateDescriptorSets(writes, {});
    }
}

void Mesh::update(const CameraState& /*input_state*/, const Transform& /*transform*/)
{
}

Mat4 Mesh::render(const RenderContext& ctx, const Mat4& world)
{
    auto extent = ctx.swapchain.extent();

    // Write UBO
    UniformBufferObject ubo{};
    ubo.model = world;
    ubo.view = Mat4::lookAt(ctx.cameraPosition, ctx.cameraTarget, {0, 1, 0});
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = ctx.cameraPosition.x();
    ubo.cameraPos[1] = ctx.cameraPosition.y();
    ubo.cameraPos[2] = ctx.cameraPosition.z();
    ubo.cameraPos[3] = 0.0f;
    memcpy(uniformMapped_[ctx.currentFrame], &ubo, sizeof(ubo));

    // Record draw commands
    auto cmd = ctx.commandBuffer;
    vk::Buffer vertBuf = *vertexBuf_;
    cmd.bindVertexBuffers(0, vertBuf, {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(*indexBuf_, 0, vk::IndexType::eUint16);
    vk::DescriptorSet ds = *descSets_[ctx.currentFrame];
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ctx.pipeline.pipelineLayout(), 0, ds,
                           {});
    cmd.drawIndexed(static_cast<uint32_t>(renderData_.indices().size()), 1, 0, 0, 0);

    return world;
}

} // namespace fire_engine
