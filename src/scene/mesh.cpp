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

Mesh::~Mesh()
{
    if (!vkDevice_)
        return;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDevice_.destroyBuffer(uniformBufs_[i]);
        vkDevice_.freeMemory(uniformMems_[i]);
        vkDevice_.destroyBuffer(materialBufs_[i]);
        vkDevice_.freeMemory(materialMems_[i]);
    }
    vkDevice_.destroyDescriptorPool(descPool_);
    texture_.destroy(vkDevice_);
    vkDevice_.destroyBuffer(indexBuf_);
    vkDevice_.freeMemory(indexMem_);
    vkDevice_.destroyBuffer(vertexBuf_);
    vkDevice_.freeMemory(vertexMem_);
}

void Mesh::load(const std::string& objPath, const std::string& mtlPath,
                const Device& device, const Pipeline& pipeline, Frame& frame)
{
    vkDevice_ = device.device();

    // Load geometry and material
    std::list<Material> materials = Material::load_from_file(mtlPath);
    Geometry geometry = Geometry::load_from_file(objPath);

    if (!materials.empty())
        material_ = materials.front();
    renderData_ = geometry.to_coloured_indexed_geometry(materials);

    // Create vertex buffer
    vk::DeviceSize vertexBufSize = sizeof(renderData_.vertices[0]) * renderData_.vertices.size();
    device.createBuffer(vertexBufSize, vk::BufferUsageFlagBits::eVertexBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible |
                            vk::MemoryPropertyFlagBits::eHostCoherent,
                        vertexBuf_, vertexMem_);
    void* vertData = vkDevice_.mapMemory(vertexMem_, 0, vertexBufSize);
    memcpy(vertData, renderData_.vertices.data(), vertexBufSize);
    vkDevice_.unmapMemory(vertexMem_);

    // Create index buffer
    vk::DeviceSize indexBufSize = sizeof(renderData_.indices[0]) * renderData_.indices.size();
    device.createBuffer(indexBufSize, vk::BufferUsageFlagBits::eIndexBuffer,
                        vk::MemoryPropertyFlagBits::eHostVisible |
                            vk::MemoryPropertyFlagBits::eHostCoherent,
                        indexBuf_, indexMem_);
    void* indexData = vkDevice_.mapMemory(indexMem_, 0, indexBufSize);
    memcpy(indexData, renderData_.indices.data(), indexBufSize);
    vkDevice_.unmapMemory(indexMem_);

    // Load texture
    std::string texPath = material_.mapKd();
    if (texPath.empty())
        texPath = "default.png";
    texture_ = Texture::load_from_file(texPath, vkDevice_, device.physicalDevice(),
                                       frame.commandPool(), device.graphicsQueue());

    // Create material uniform buffers
    vk::DeviceSize matSize = sizeof(MaterialUBO);
    materialBufs_.resize(MAX_FRAMES_IN_FLIGHT);
    materialMems_.resize(MAX_FRAMES_IN_FLIGHT);
    materialMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        device.createBuffer(matSize, vk::BufferUsageFlagBits::eUniformBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible |
                                vk::MemoryPropertyFlagBits::eHostCoherent,
                            materialBufs_[i], materialMems_[i]);
        materialMapped_[i] = vkDevice_.mapMemory(materialMems_[i], 0, matSize);

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
    createDescriptorSets(pipeline);
}

void Mesh::createUniformBuffers(const Device& device)
{
    vk::DeviceSize size = sizeof(UniformBufferObject);
    uniformBufs_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformMems_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        device.createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
                            vk::MemoryPropertyFlagBits::eHostVisible |
                                vk::MemoryPropertyFlagBits::eHostCoherent,
                            uniformBufs_[i], uniformMems_[i]);
        uniformMapped_[i] = vkDevice_.mapMemory(uniformMems_[i], 0, size);
    }
}

void Mesh::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2)},
        {vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
    }};
    vk::DescriptorPoolCreateInfo ci({}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), poolSizes);
    descPool_ = vkDevice_.createDescriptorPool(ci);
}

void Mesh::createDescriptorSets(const Pipeline& pipeline)
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 pipeline.descriptorSetLayout());
    vk::DescriptorSetAllocateInfo ai(descPool_, layouts);
    descSets_ = vkDevice_.allocateDescriptorSets(ai);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo uboBufInfo(uniformBufs_[i], 0, sizeof(UniformBufferObject));
        vk::DescriptorBufferInfo matBufInfo(materialBufs_[i], 0, sizeof(MaterialUBO));
        vk::DescriptorImageInfo texInfo(texture_.sampler(), texture_.view(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal);

        std::array<vk::WriteDescriptorSet, 3> writes = {{
            {descSets_[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uboBufInfo},
            {descSets_[i], 1, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &matBufInfo},
            {descSets_[i], 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &texInfo},
        }};
        vkDevice_.updateDescriptorSets(writes, {});
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
    cmd.bindVertexBuffers(0, vertexBuf_, {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(indexBuf_, 0, vk::IndexType::eUint16);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, ctx.pipeline.pipelineLayout(), 0,
                           descSets_[ctx.currentFrame], {});
    cmd.drawIndexed(static_cast<uint32_t>(renderData_.indices.size()), 1, 0, 0, 0);

    return world;
}

} // namespace fire_engine
