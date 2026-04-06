#include <fire_engine/renderer/frame.hpp>

#include <fire_engine/math/constants.hpp>
#include <fire_engine/math/mat4.hpp>

namespace fire_engine
{

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

// ---------------------------------------------------------------------------
// UBO matching shader.vert layout(binding = 0)
// ---------------------------------------------------------------------------
struct UniformBufferObject
{
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    alignas(16) float cameraPos[4];
};

struct MaterialUBO
{
    alignas(16) float ambient[3];
    alignas(16) float diffuse[3];
    alignas(16) float specular[3];
    alignas(16) float emissive[3];
    float shininess;
    float ior;
    float transparency;
    int illum;
    float roughness;
    float metallic;
    float sheen;
    float clearcoat;
    float clearcoatRoughness;
    float anisotropy;
    float anisotropyRotation;
    float _pad0;
};

Frame::Frame(const Device& device, Swapchain& swapchain, const Pipeline& pipeline)
    : device_(&device),
      swapchain_(&swapchain),
      pipeline_(&pipeline),
      vkDevice_(device.device())
{
    createCommandPool();
    createGeometryBuffer();
    createTexture();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

Frame::~Frame()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDevice_.destroySemaphore(imageAvail_[i]);
        vkDevice_.destroyFence(inFlight_[i]);
        vkDevice_.destroyBuffer(uniformBufs_[i]);
        vkDevice_.freeMemory(uniformMems_[i]);
        vkDevice_.destroyBuffer(materialBufs_[i]);
        vkDevice_.freeMemory(materialMems_[i]);
    }
    texture_.destroy(vkDevice_);
    vkDevice_.destroyDescriptorPool(descPool_);
    vkDevice_.destroyBuffer(indexBuf_);
    vkDevice_.freeMemory(indexMem_);
    vkDevice_.destroyBuffer(vertexBuf_);
    vkDevice_.freeMemory(vertexMem_);
    vkDevice_.destroyCommandPool(cmdPool_);
    for (auto sem : renderDone_)
        vkDevice_.destroySemaphore(sem);
}

void Frame::createCommandPool()
{
    vk::CommandPoolCreateInfo ci(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                 device_->graphicsFamily());
    cmdPool_ = vkDevice_.createCommandPool(ci);
}

void Frame::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                         vk::MemoryPropertyFlags props, vk::Buffer& buf, vk::DeviceMemory& mem)
{
    vk::BufferCreateInfo ci({}, size, usage, vk::SharingMode::eExclusive);
    buf = vkDevice_.createBuffer(ci);

    auto req = vkDevice_.getBufferMemoryRequirements(buf);
    vk::MemoryAllocateInfo ai(req.size, device_->findMemoryType(req.memoryTypeBits, props));
    mem = vkDevice_.allocateMemory(ai);
    vkDevice_.bindBufferMemory(buf, mem, 0);
}

void Frame::createGeometryBuffer()
{
    std::list<Material> materials = Material::load_from_file("capsule.mtl");
    Geometry geometry = Geometry::load_from_file("capsule.obj");

    if (!materials.empty())
        material_ = materials.front();
    renderData_ = geometry.to_coloured_indexed_geometry(materials);

    vk::DeviceSize vertexBufSize = sizeof(renderData_.vertices[0]) * renderData_.vertices.size();
    createBuffer(vertexBufSize, vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 vertexBuf_, vertexMem_);
    void* vertData = vkDevice_.mapMemory(vertexMem_, 0, vertexBufSize);
    memcpy(vertData, renderData_.vertices.data(), vertexBufSize);
    vkDevice_.unmapMemory(vertexMem_);

    vk::DeviceSize indexBufSize = sizeof(renderData_.indices[0]) * renderData_.indices.size();
    createBuffer(indexBufSize, vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 indexBuf_, indexMem_);
    void* indexData = vkDevice_.mapMemory(indexMem_, 0, indexBufSize);
    memcpy(indexData, renderData_.indices.data(), indexBufSize);
    vkDevice_.unmapMemory(indexMem_);
}

void Frame::createTexture()
{
    std::string texPath = material_.mapKd();
    if (texPath.empty())
        texPath = "default.png";
    texture_ = Texture::load_from_file(texPath, vkDevice_, device_->physicalDevice(), cmdPool_,
                                       device_->graphicsQueue());
}

void Frame::createUniformBuffers()
{
    vk::DeviceSize size = sizeof(UniformBufferObject);
    uniformBufs_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformMems_.resize(MAX_FRAMES_IN_FLIGHT);
    uniformMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent,
                     uniformBufs_[i], uniformMems_[i]);
        uniformMapped_[i] = vkDevice_.mapMemory(uniformMems_[i], 0, size);
    }

    vk::DeviceSize matSize = sizeof(MaterialUBO);
    materialBufs_.resize(MAX_FRAMES_IN_FLIGHT);
    materialMems_.resize(MAX_FRAMES_IN_FLIGHT);
    materialMapped_.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(matSize, vk::BufferUsageFlagBits::eUniformBuffer,
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
}

void Frame::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2)},
        {vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
    }};
    vk::DescriptorPoolCreateInfo ci({}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), poolSizes);
    descPool_ = vkDevice_.createDescriptorPool(ci);
}

void Frame::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 pipeline_->descriptorSetLayout());
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

void Frame::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo ai(cmdPool_, vk::CommandBufferLevel::ePrimary,
                                     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    cmdBufs_ = vkDevice_.allocateCommandBuffers(ai);
}

void Frame::createSyncObjects()
{
    imageAvail_.resize(MAX_FRAMES_IN_FLIGHT);
    renderDone_.resize(swapchain_->images().size());
    inFlight_.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo sci;
    vk::FenceCreateInfo fci(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvail_[i] = vkDevice_.createSemaphore(sci);
        inFlight_[i] = vkDevice_.createFence(fci);
    }
    for (size_t i = 0; i < swapchain_->images().size(); ++i)
    {
        renderDone_[i] = vkDevice_.createSemaphore(sci);
    }
}

void Frame::destroyRenderFinishedSemaphores()
{
    for (auto sem : renderDone_)
        vkDevice_.destroySemaphore(sem);
    renderDone_.clear();
}

void Frame::createRenderFinishedSemaphores(size_t count)
{
    vk::SemaphoreCreateInfo sci;
    renderDone_.resize(count);
    for (size_t i = 0; i < count; ++i)
        renderDone_[i] = vkDevice_.createSemaphore(sci);
}

} // namespace fire_engine
