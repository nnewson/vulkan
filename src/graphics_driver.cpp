#include <string>

#include <fire_engine/core/shader_loader.hpp>
#include <fire_engine/core/system.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/math/mat4.hpp>

#include <fire_engine/graphics_driver.hpp>

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

GraphicsDriver::GraphicsDriver(Renderer& renderer)
    : renderer_(&renderer)
{
}

GraphicsDriver::~GraphicsDriver()
{
    auto dev = renderer_->device().device();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        dev.destroySemaphore(imageAvail_[i]);
        dev.destroyFence(inFlight_[i]);
        dev.destroyBuffer(uniformBufs_[i]);
        dev.freeMemory(uniformMems_[i]);
        dev.destroyBuffer(materialBufs_[i]);
        dev.freeMemory(materialMems_[i]);
    }
    texture_.destroy(dev);
    dev.destroyDescriptorPool(descPool_);
    dev.destroyBuffer(indexBuf_);
    dev.freeMemory(indexMem_);
    dev.destroyBuffer(vertexBuf_);
    dev.freeMemory(vertexMem_);
    dev.destroyCommandPool(cmdPool_);
    for (auto sem : renderDone_)
        dev.destroySemaphore(sem);
    dev.destroyPipeline(pipeline_);
    dev.destroyPipelineLayout(pipelineLayout_);
    dev.destroyRenderPass(renderPass_);
    dev.destroyDescriptorSetLayout(descSetLayout_);
}

void GraphicsDriver::init()
{
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    renderer_->swapchain().createDepthResources(renderer_->device());
    renderer_->swapchain().createFramebuffers(renderPass_);
    createCommandPool();
    createGeometryBuffer();
    createTexture();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void GraphicsDriver::createRenderPass()
{
    vk::AttachmentDescription colorAtt(
        {}, renderer_->swapchain().format(), vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAtt(
        {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorRef, {},
                                   &depthRef);

    vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              {},
                              vk::AccessFlagBits::eColorAttachmentWrite |
                                  vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 2> attachments = {colorAtt, depthAtt};
    vk::RenderPassCreateInfo ci({}, attachments, subpass, dep);
    renderPass_ = renderer_->device().device().createRenderPass(ci);
}

void GraphicsDriver::createDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {{
        {0, vk::DescriptorType::eUniformBuffer, 1,
         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
        {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
        {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    }};

    vk::DescriptorSetLayoutCreateInfo ci({}, bindings);
    descSetLayout_ = renderer_->device().device().createDescriptorSetLayout(ci);
}

vk::ShaderModule GraphicsDriver::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo ci({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));
    return renderer_->device().device().createShaderModule(ci);
}

void GraphicsDriver::createGraphicsPipeline()
{
    auto dev = renderer_->device().device();
    auto extent = renderer_->swapchain().extent();

    auto vertCode = ShaderLoader::load_from_file("shader.vert.spv");
    auto fragCode = ShaderLoader::load_from_file("shader.frag.spv");
    vk::ShaderModule vertMod = createShaderModule(vertCode);
    vk::ShaderModule fragMod = createShaderModule(fragCode);

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {{
        {{}, vk::ShaderStageFlagBits::eVertex, vertMod, "main"},
        {{}, vk::ShaderStageFlagBits::eFragment, fragMod, "main"},
    }};

    auto bindDesc = Vertex::bindingDesc();
    auto attrDesc = Vertex::attrDescs();
    vk::PipelineVertexInputStateCreateInfo vertInput({}, bindDesc, attrDesc);

    vk::PipelineInputAssemblyStateCreateInfo inputAsm({}, vk::PrimitiveTopology::eTriangleList);

    vk::Viewport viewport(0, 0, static_cast<float>(extent.width),
                          static_cast<float>(extent.height), 0, 1);
    vk::Rect2D scissor({0, 0}, extent);
    vk::PipelineViewportStateCreateInfo vpState({}, viewport, scissor);

    vk::PipelineRasterizationStateCreateInfo raster(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise, false, 0, 0, 0, 1.0f);

    vk::PipelineMultisampleStateCreateInfo ms({}, vk::SampleCountFlagBits::e1);

    vk::PipelineDepthStencilStateCreateInfo depthStencil({}, true, true, vk::CompareOp::eLess);

    vk::PipelineColorBlendAttachmentState colorBlendAtt(
        false, {}, {}, {}, {}, {}, {},
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, {}, colorBlendAtt);

    vk::PipelineLayoutCreateInfo plci({}, descSetLayout_);
    pipelineLayout_ = dev.createPipelineLayout(plci);

    vk::GraphicsPipelineCreateInfo pci({}, stages, &vertInput, &inputAsm, nullptr, &vpState,
                                       &raster, &ms, &depthStencil, &colorBlend, nullptr,
                                       pipelineLayout_, renderPass_, 0);

    auto result = dev.createGraphicsPipeline(nullptr, pci);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("failed to create graphics pipeline");
    pipeline_ = result.value;

    dev.destroyShaderModule(fragMod);
    dev.destroyShaderModule(vertMod);
}

void GraphicsDriver::createCommandPool()
{
    vk::CommandPoolCreateInfo ci(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                 renderer_->device().graphicsFamily());
    cmdPool_ = renderer_->device().device().createCommandPool(ci);
}

void GraphicsDriver::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                  vk::MemoryPropertyFlags props, vk::Buffer& buf,
                                  vk::DeviceMemory& mem)
{
    auto dev = renderer_->device().device();
    vk::BufferCreateInfo ci({}, size, usage, vk::SharingMode::eExclusive);
    buf = dev.createBuffer(ci);

    auto req = dev.getBufferMemoryRequirements(buf);
    vk::MemoryAllocateInfo ai(req.size, renderer_->device().findMemoryType(req.memoryTypeBits, props));
    mem = dev.allocateMemory(ai);
    dev.bindBufferMemory(buf, mem, 0);
}

void GraphicsDriver::createGeometryBuffer()
{
    auto dev = renderer_->device().device();

    // std::list<Material> materials = Material::load_from_file("cube.mtl");
    // std::list<Material> materials = Material::load_from_file("utah_blend.mtl");
    // std::list<Material> materials = Material::load_from_file("default.mtl");
    std::list<Material> materials = Material::load_from_file("capsule.mtl");
    // Geometry geometry = Geometry::load_from_file("cube.obj");
    // Geometry geometry = Geometry::load_from_file("utah_blend.obj");
    // Geometry geometry = Geometry::load_from_file("teapot.obj");
    Geometry geometry = Geometry::load_from_file("capsule.obj");

    if (!materials.empty())
        material_ = materials.front();
    renderData_ = geometry.to_coloured_indexed_geometry(materials);

    vk::DeviceSize vertexBufSize = sizeof(renderData_.vertices[0]) * renderData_.vertices.size();
    createBuffer(vertexBufSize, vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 vertexBuf_, vertexMem_);
    void* vertData = dev.mapMemory(vertexMem_, 0, vertexBufSize);
    memcpy(vertData, renderData_.vertices.data(), vertexBufSize);
    dev.unmapMemory(vertexMem_);

    vk::DeviceSize indexBufSize = sizeof(renderData_.indices[0]) * renderData_.indices.size();
    createBuffer(indexBufSize, vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 indexBuf_, indexMem_);
    void* indexData = dev.mapMemory(indexMem_, 0, indexBufSize);
    memcpy(indexData, renderData_.indices.data(), indexBufSize);
    dev.unmapMemory(indexMem_);
}

void GraphicsDriver::createTexture()
{
    std::string texPath = material_.mapKd();
    if (texPath.empty())
        texPath = "default.png";
    texture_ = Texture::load_from_file(texPath, renderer_->device().device(),
                                       renderer_->device().physicalDevice(), cmdPool_,
                                       renderer_->device().graphicsQueue());
}

void GraphicsDriver::createUniformBuffers()
{
    auto dev = renderer_->device().device();

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
        uniformMapped_[i] = dev.mapMemory(uniformMems_[i], 0, size);
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
        materialMapped_[i] = dev.mapMemory(materialMems_[i], 0, matSize);

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

void GraphicsDriver::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {{
        {vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2)},
        {vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
    }};
    vk::DescriptorPoolCreateInfo ci({}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), poolSizes);
    descPool_ = renderer_->device().device().createDescriptorPool(ci);
}

void GraphicsDriver::createDescriptorSets()
{
    auto dev = renderer_->device().device();

    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descSetLayout_);
    vk::DescriptorSetAllocateInfo ai(descPool_, layouts);
    descSets_ = dev.allocateDescriptorSets(ai);

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
        dev.updateDescriptorSets(writes, {});
    }
}

void GraphicsDriver::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo ai(cmdPool_, vk::CommandBufferLevel::ePrimary,
                                     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    cmdBufs_ = renderer_->device().device().allocateCommandBuffers(ai);
}

void GraphicsDriver::recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex)
{
    auto extent = renderer_->swapchain().extent();

    cmd.begin(vk::CommandBufferBeginInfo{});

    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue(std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f});
    clears[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo rpBegin(renderPass_, renderer_->swapchain().framebuffers()[imageIndex],
                                    vk::Rect2D({0, 0}, extent), clears);

    cmd.beginRenderPass(rpBegin, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
    cmd.bindVertexBuffers(0, vertexBuf_, {vk::DeviceSize{0}});
    cmd.bindIndexBuffer(indexBuf_, 0, vk::IndexType::eUint16);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout_, 0,
                           descSets_[currentFrame_], {});
    cmd.drawIndexed(static_cast<uint32_t>(renderData_.indices.size()), 1, 0, 0, 0);
    cmd.endRenderPass();
    cmd.end();
}

void GraphicsDriver::createSyncObjects()
{
    auto dev = renderer_->device().device();

    imageAvail_.resize(MAX_FRAMES_IN_FLIGHT);
    renderDone_.resize(renderer_->swapchain().images().size());
    inFlight_.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo sci;
    vk::FenceCreateInfo fci(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvail_[i] = dev.createSemaphore(sci);
        inFlight_[i] = dev.createFence(fci);
    }
    for (size_t i = 0; i < renderer_->swapchain().images().size(); ++i)
    {
        renderDone_[i] = dev.createSemaphore(sci);
    }
}

void GraphicsDriver::updateUniformBuffer(Vec3 cameraPos, Vec3 cameraTarget)
{
    static auto startTime = System::getTime();
    float t = static_cast<float>(System::getTime() - startTime);

    auto extent = renderer_->swapchain().extent();

    UniformBufferObject ubo{};
    ubo.model = Mat4::rotateY(t * 1.0f) * Mat4::rotateX(t * 0.5f);
    // ubo.model = Mat4::identity();
    ubo.view = Mat4::lookAt(cameraPos, cameraTarget, {0, 1, 0});
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = cameraPos.x();
    ubo.cameraPos[1] = cameraPos.y();
    ubo.cameraPos[2] = cameraPos.z();
    ubo.cameraPos[3] = 0.0f;

    memcpy(uniformMapped_[currentFrame_], &ubo, sizeof(ubo));
}

void GraphicsDriver::recreateSwapchain(const Window& display)
{
    int w = 0, h = 0;
    glfwGetFramebufferSize(display.getWindow(), &w, &h);
    while (w == 0 || h == 0)
    {
        glfwGetFramebufferSize(display.getWindow(), &w, &h);
        glfwWaitEvents();
    }
    waitIdle();

    auto dev = renderer_->device().device();
    for (auto sem : renderDone_)
        dev.destroySemaphore(sem);
    renderDone_.clear();

    renderer_->swapchain().recreate(renderer_->device(), display, renderPass_);

    vk::SemaphoreCreateInfo sci;
    renderDone_.resize(renderer_->swapchain().images().size());
    for (size_t i = 0; i < renderer_->swapchain().images().size(); ++i)
        renderDone_[i] = dev.createSemaphore(sci);
}

void GraphicsDriver::drawFrame(Window& display, Vec3 cameraPos, Vec3 cameraTarget)
{
    auto dev = renderer_->device().device();

    (void)dev.waitForFences(inFlight_[currentFrame_], vk::True, UINT64_MAX);

    auto [acquireResult, imageIndex] =
        dev.acquireNextImageKHR(renderer_->swapchain().swapchain(), UINT64_MAX,
                                imageAvail_[currentFrame_]);
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain(display);
        return;
    }
    if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("failed to acquire swap chain image");

    dev.resetFences(inFlight_[currentFrame_]);

    updateUniformBuffer(cameraPos, cameraTarget);

    cmdBufs_[currentFrame_].reset();
    recordCommandBuffer(cmdBufs_[currentFrame_], imageIndex);

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo si(imageAvail_[currentFrame_], waitStage, cmdBufs_[currentFrame_],
                      renderDone_[imageIndex]);

    renderer_->device().graphicsQueue().submit(si, inFlight_[currentFrame_]);

    auto swapchain = renderer_->swapchain().swapchain();
    vk::PresentInfoKHR pi(renderDone_[imageIndex], swapchain, imageIndex);

    vk::Result presentResult = renderer_->device().presentQueue().presentKHR(pi);
    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR || display.framebufferResized())
    {
        display.framebufferResized(false);
        recreateSwapchain(display);
    }
    else if (presentResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicsDriver::waitIdle()
{
    renderer_->device().device().waitIdle();
}

} // namespace fire_engine
