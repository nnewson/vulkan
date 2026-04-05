#include <fstream>
#include <set>
#include <string>

#include <fire_engine/display.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/math/mat4.hpp>

#include <fire_engine/graphics_driver.hpp>

namespace fire_engine
{

#ifdef NDEBUG
constexpr bool enableValidation = false;
#else
constexpr bool enableValidation = true;
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::vector<char> readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("failed to open file: " + path);
    size_t sz = static_cast<size_t>(f.tellg());
    std::vector<char> buf(sz);
    f.seekg(0);
    f.read(buf.data(), static_cast<std::streamsize>(sz));
    return buf;
}

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

// ---------------------------------------------------------------------------
// Validation / device extension lists
// ---------------------------------------------------------------------------
static const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
static const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset", // required on macOS/MoltenVK
};

GraphicsDriver::GraphicsDriver()
{
    vk::ApplicationInfo appInfo("Vulkan Cube", VK_MAKE_VERSION(1, 0, 0), "No Engine",
                                VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> exts(glfwExts, glfwExts + glfwExtCount);
    exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    exts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    std::vector<const char*> layers;
    if (enableValidation)
        layers = validationLayers;

    vk::InstanceCreateInfo ci(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &appInfo,
                              layers, exts);

    instance_ = vk::createInstance(ci);
}

GraphicsDriver::~GraphicsDriver()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        device_.destroySemaphore(imageAvail_[i]);
        device_.destroyFence(inFlight_[i]);
        device_.destroyBuffer(uniformBufs_[i]);
        device_.freeMemory(uniformMems_[i]);
        device_.destroyBuffer(materialBufs_[i]);
        device_.freeMemory(materialMems_[i]);
    }
    texture_.destroy(device_);
    device_.destroyDescriptorPool(descPool_);
    device_.destroyBuffer(indexBuf_);
    device_.freeMemory(indexMem_);
    device_.destroyBuffer(vertexBuf_);
    device_.freeMemory(vertexMem_);
    device_.destroyCommandPool(cmdPool_);
    cleanupSwapchain();
    device_.destroyPipeline(pipeline_);
    device_.destroyPipelineLayout(pipelineLayout_);
    device_.destroyRenderPass(renderPass_);
    device_.destroyDescriptorSetLayout(descSetLayout_);
    device_.destroy();

    instance_.destroySurfaceKHR(surface_);
    instance_.destroy();
}

void GraphicsDriver::init(const Display& display)
{
    createSurface(display);
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain(display);
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createGeometryBuffer();
    createTexture();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void GraphicsDriver::createSurface(const Display& display)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance_, display.getWindow(), nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface");
    surface_ = surface;
}

void GraphicsDriver::pickPhysicalDevice()
{
    auto devs = instance_.enumeratePhysicalDevices();
    for (auto& d : devs)
    {
        if (isDeviceSuitable(d))
        {
            physDevice_ = d;
            return;
        }
    }
    throw std::runtime_error("no suitable GPU found");
}

bool GraphicsDriver::isDeviceSuitable(vk::PhysicalDevice d)
{
    auto [gf, pf] = findQueueFamilies(d);
    if (!gf.has_value() || !pf.has_value())
        return false;

    auto avail = d.enumerateDeviceExtensionProperties();
    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
    for (auto& e : avail)
        required.erase(e.extensionName);
    if (!required.empty())
        return false;

    auto fmts = d.getSurfaceFormatsKHR(surface_);
    auto modes = d.getSurfacePresentModesKHR(surface_);
    return !fmts.empty() && !modes.empty();
}

std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
GraphicsDriver::findQueueFamilies(vk::PhysicalDevice d)
{
    auto families = d.getQueueFamilyProperties();
    std::optional<uint32_t> gf, pf;
    for (uint32_t i = 0; i < families.size(); ++i)
    {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
            gf = i;
        if (d.getSurfaceSupportKHR(i, surface_))
            pf = i;
        if (gf && pf)
            break;
    }
    return {gf, pf};
}

void GraphicsDriver::createLogicalDevice()
{
    auto [gf, pf] = findQueueFamilies(physDevice_);
    graphicsFamily_ = gf.value();
    presentFamily_ = pf.value();

    std::set<uint32_t> uniqueFamilies = {graphicsFamily_, presentFamily_};
    std::vector<vk::DeviceQueueCreateInfo> qcis;
    float prio = 1.0f;
    for (uint32_t fam : uniqueFamilies)
    {
        qcis.emplace_back(vk::DeviceQueueCreateFlags{}, fam, 1, &prio);
    }

    vk::PhysicalDeviceFeatures features{};
    features.samplerAnisotropy = vk::True;

    vk::DeviceCreateInfo ci({}, qcis, {}, deviceExtensions, &features);

    device_ = physDevice_.createDevice(ci);
    graphicsQueue_ = device_.getQueue(graphicsFamily_, 0);
    presentQueue_ = device_.getQueue(presentFamily_, 0);
}

vk::SurfaceFormatKHR GraphicsDriver::chooseSwapFormat()
{
    auto fmts = physDevice_.getSurfaceFormatsKHR(surface_);
    for (auto& f : fmts)
        if (f.format == vk::Format::eB8G8R8A8Srgb &&
            f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return f;
    return fmts[0];
}

vk::PresentModeKHR GraphicsDriver::chooseSwapPresentMode()
{
    auto modes = physDevice_.getSurfacePresentModesKHR(surface_);
    for (auto& m : modes)
        if (m == vk::PresentModeKHR::eMailbox)
            return m;
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D GraphicsDriver::chooseSwapExtent(const Display& display,
                                              const vk::SurfaceCapabilitiesKHR& caps)
{
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;
    int w, h;
    glfwGetFramebufferSize(display.getWindow(), &w, &h);
    vk::Extent2D ext(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    ext.width = std::clamp(ext.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    ext.height = std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return ext;
}

void GraphicsDriver::createSwapchain(const Display& display)
{
    auto caps = physDevice_.getSurfaceCapabilitiesKHR(surface_);
    auto fmt = chooseSwapFormat();
    auto mode = chooseSwapPresentMode();
    auto extent = chooseSwapExtent(display, caps);

    uint32_t imgCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0)
        imgCount = std::min(imgCount, caps.maxImageCount);

    uint32_t families[] = {graphicsFamily_, presentFamily_};
    bool concurrent = graphicsFamily_ != presentFamily_;

    vk::SwapchainCreateInfoKHR ci(
        {}, surface_, imgCount, fmt.format, fmt.colorSpace, extent, 1,
        vk::ImageUsageFlagBits::eColorAttachment,
        concurrent ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        concurrent ? 2u : 0u, concurrent ? families : nullptr, caps.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque, mode, vk::True);

    swapchain_ = device_.createSwapchainKHR(ci);
    swapImages_ = device_.getSwapchainImagesKHR(swapchain_);
    swapFormat_ = fmt.format;
    swapExtent_ = extent;
}

void GraphicsDriver::createImageViews()
{
    swapViews_.resize(swapImages_.size());
    for (size_t i = 0; i < swapImages_.size(); ++i)
        swapViews_[i] =
            createImageView(swapImages_[i], swapFormat_, vk::ImageAspectFlagBits::eColor);
}

void GraphicsDriver::createRenderPass()
{
    vk::AttachmentDescription colorAtt(
        {}, swapFormat_, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

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
    renderPass_ = device_.createRenderPass(ci);
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
    descSetLayout_ = device_.createDescriptorSetLayout(ci);
}

vk::ShaderModule GraphicsDriver::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo ci({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));
    return device_.createShaderModule(ci);
}

void GraphicsDriver::createGraphicsPipeline()
{
    auto vertCode = readFile("shader.vert.spv");
    auto fragCode = readFile("shader.frag.spv");
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

    vk::Viewport viewport(0, 0, static_cast<float>(swapExtent_.width),
                          static_cast<float>(swapExtent_.height), 0, 1);
    vk::Rect2D scissor({0, 0}, swapExtent_);
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
    pipelineLayout_ = device_.createPipelineLayout(plci);

    vk::GraphicsPipelineCreateInfo pci({}, stages, &vertInput, &inputAsm, nullptr, &vpState,
                                       &raster, &ms, &depthStencil, &colorBlend, nullptr,
                                       pipelineLayout_, renderPass_, 0);

    auto result = device_.createGraphicsPipeline(nullptr, pci);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("failed to create graphics pipeline");
    pipeline_ = result.value;

    device_.destroyShaderModule(fragMod);
    device_.destroyShaderModule(vertMod);
}

uint32_t GraphicsDriver::findMemoryType(uint32_t filter, vk::MemoryPropertyFlags props)
{
    auto mem = physDevice_.getMemoryProperties();
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
        if ((filter & (1 << i)) && (mem.memoryTypes[i].propertyFlags & props) == props)
            return i;
    throw std::runtime_error("failed to find suitable memory type");
}

vk::ImageView GraphicsDriver::createImageView(vk::Image img, vk::Format fmt,
                                              vk::ImageAspectFlags aspect)
{
    vk::ImageViewCreateInfo ci({}, img, vk::ImageViewType::e2D, fmt, {},
                               vk::ImageSubresourceRange(aspect, 0, 1, 0, 1));
    return device_.createImageView(ci);
}

void GraphicsDriver::createDepthResources()
{
    vk::Format depthFmt = vk::Format::eD32Sfloat;
    vk::ImageCreateInfo ci({}, vk::ImageType::e2D, depthFmt,
                           vk::Extent3D(swapExtent_.width, swapExtent_.height, 1), 1, 1,
                           vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                           vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    depthImage_ = device_.createImage(ci);

    auto req = device_.getImageMemoryRequirements(depthImage_);
    vk::MemoryAllocateInfo ai(
        req.size, findMemoryType(req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    depthMem_ = device_.allocateMemory(ai);
    device_.bindImageMemory(depthImage_, depthMem_, 0);
    depthView_ = createImageView(depthImage_, depthFmt, vk::ImageAspectFlagBits::eDepth);
}

void GraphicsDriver::createFramebuffers()
{
    framebuffers_.resize(swapViews_.size());
    for (size_t i = 0; i < swapViews_.size(); ++i)
    {
        std::array<vk::ImageView, 2> attachments = {swapViews_[i], depthView_};
        vk::FramebufferCreateInfo ci({}, renderPass_, attachments, swapExtent_.width,
                                     swapExtent_.height, 1);
        framebuffers_[i] = device_.createFramebuffer(ci);
    }
}

void GraphicsDriver::createCommandPool()
{
    vk::CommandPoolCreateInfo ci(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                 graphicsFamily_);
    cmdPool_ = device_.createCommandPool(ci);
}

void GraphicsDriver::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                  vk::MemoryPropertyFlags props, vk::Buffer& buf,
                                  vk::DeviceMemory& mem)
{
    vk::BufferCreateInfo ci({}, size, usage, vk::SharingMode::eExclusive);
    buf = device_.createBuffer(ci);

    auto req = device_.getBufferMemoryRequirements(buf);
    vk::MemoryAllocateInfo ai(req.size, findMemoryType(req.memoryTypeBits, props));
    mem = device_.allocateMemory(ai);
    device_.bindBufferMemory(buf, mem, 0);
}

void GraphicsDriver::createGeometryBuffer()
{
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
    void* vertData = device_.mapMemory(vertexMem_, 0, vertexBufSize);
    memcpy(vertData, renderData_.vertices.data(), vertexBufSize);
    device_.unmapMemory(vertexMem_);

    vk::DeviceSize indexBufSize = sizeof(renderData_.indices[0]) * renderData_.indices.size();
    createBuffer(indexBufSize, vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 indexBuf_, indexMem_);
    void* indexData = device_.mapMemory(indexMem_, 0, indexBufSize);
    memcpy(indexData, renderData_.indices.data(), indexBufSize);
    device_.unmapMemory(indexMem_);
}

void GraphicsDriver::createTexture()
{
    std::string texPath = material_.mapKd;
    if (texPath.empty())
        texPath = "default.png";
    texture_ = Texture::load_from_file(texPath, device_, physDevice_, cmdPool_, graphicsQueue_);
}

void GraphicsDriver::createUniformBuffers()
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
        uniformMapped_[i] = device_.mapMemory(uniformMems_[i], 0, size);
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
        materialMapped_[i] = device_.mapMemory(materialMems_[i], 0, matSize);

        MaterialUBO matUbo{};
        matUbo.ambient[0] = material_.ambient.r();
        matUbo.ambient[1] = material_.ambient.g();
        matUbo.ambient[2] = material_.ambient.b();
        matUbo.diffuse[0] = material_.diffuse.r();
        matUbo.diffuse[1] = material_.diffuse.g();
        matUbo.diffuse[2] = material_.diffuse.b();
        matUbo.specular[0] = material_.specular.r();
        matUbo.specular[1] = material_.specular.g();
        matUbo.specular[2] = material_.specular.b();
        matUbo.emissive[0] = material_.emissive.r();
        matUbo.emissive[1] = material_.emissive.g();
        matUbo.emissive[2] = material_.emissive.b();
        matUbo.shininess = material_.shininess;
        matUbo.ior = material_.ior;
        matUbo.transparency = material_.transparency;
        matUbo.illum = material_.illum;
        matUbo.roughness = material_.roughness;
        matUbo.metallic = material_.metallic;
        matUbo.sheen = material_.sheen;
        matUbo.clearcoat = material_.clearcoat;
        matUbo.clearcoatRoughness = material_.clearcoatRoughness;
        matUbo.anisotropy = material_.anisotropy;
        matUbo.anisotropyRotation = material_.anisotropyRotation;
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
    descPool_ = device_.createDescriptorPool(ci);
}

void GraphicsDriver::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descSetLayout_);
    vk::DescriptorSetAllocateInfo ai(descPool_, layouts);
    descSets_ = device_.allocateDescriptorSets(ai);

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
        device_.updateDescriptorSets(writes, {});
    }
}

void GraphicsDriver::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo ai(cmdPool_, vk::CommandBufferLevel::ePrimary,
                                     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    cmdBufs_ = device_.allocateCommandBuffers(ai);
}

void GraphicsDriver::recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex)
{
    cmd.begin(vk::CommandBufferBeginInfo{});

    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue(std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f});
    clears[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo rpBegin(renderPass_, framebuffers_[imageIndex],
                                    vk::Rect2D({0, 0}, swapExtent_), clears);

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
    imageAvail_.resize(MAX_FRAMES_IN_FLIGHT);
    renderDone_.resize(swapImages_.size());
    inFlight_.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo sci;
    vk::FenceCreateInfo fci(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvail_[i] = device_.createSemaphore(sci);
        inFlight_[i] = device_.createFence(fci);
    }
    for (size_t i = 0; i < swapImages_.size(); ++i)
    {
        renderDone_[i] = device_.createSemaphore(sci);
    }
}

void GraphicsDriver::updateUniformBuffer(Vec3 cameraPos, Vec3 cameraTarget)
{
    static auto startTime = glfwGetTime();
    float t = static_cast<float>(glfwGetTime() - startTime);

    UniformBufferObject ubo{};
    ubo.model = Mat4::rotateY(t * 1.0f) * Mat4::rotateX(t * 0.5f);
    // ubo.model = Mat4::identity();
    ubo.view = Mat4::lookAt(cameraPos, cameraTarget, {0, 1, 0});
    float aspect = static_cast<float>(swapExtent_.width) / static_cast<float>(swapExtent_.height);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = cameraPos.x();
    ubo.cameraPos[1] = cameraPos.y();
    ubo.cameraPos[2] = cameraPos.z();
    ubo.cameraPos[3] = 0.0f;

    memcpy(uniformMapped_[currentFrame_], &ubo, sizeof(ubo));
}

void GraphicsDriver::cleanupSwapchain()
{
    device_.destroyImageView(depthView_);
    device_.destroyImage(depthImage_);
    device_.freeMemory(depthMem_);
    for (auto fb : framebuffers_)
        device_.destroyFramebuffer(fb);
    for (auto iv : swapViews_)
        device_.destroyImageView(iv);
    for (auto sem : renderDone_)
        device_.destroySemaphore(sem);
    renderDone_.clear();
    device_.destroySwapchainKHR(swapchain_);
}

void GraphicsDriver::recreateSwapchain(const Display& display)
{
    int w = 0, h = 0;
    glfwGetFramebufferSize(display.getWindow(), &w, &h);
    while (w == 0 || h == 0)
    {
        glfwGetFramebufferSize(display.getWindow(), &w, &h);
        glfwWaitEvents();
    }
    waitIdle();
    cleanupSwapchain();
    createSwapchain(display);
    createImageViews();
    createDepthResources();
    createFramebuffers();

    vk::SemaphoreCreateInfo sci;
    renderDone_.resize(swapImages_.size());
    for (size_t i = 0; i < swapImages_.size(); ++i)
        renderDone_[i] = device_.createSemaphore(sci);
}

void GraphicsDriver::drawFrame(const Display& display, Vec3 cameraPos, Vec3 cameraTarget)
{
    (void)device_.waitForFences(inFlight_[currentFrame_], vk::True, UINT64_MAX);

    auto [acquireResult, imageIndex] =
        device_.acquireNextImageKHR(swapchain_, UINT64_MAX, imageAvail_[currentFrame_]);
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain(display);
        return;
    }
    if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("failed to acquire swap chain image");

    device_.resetFences(inFlight_[currentFrame_]);

    updateUniformBuffer(cameraPos, cameraTarget);

    cmdBufs_[currentFrame_].reset();
    recordCommandBuffer(cmdBufs_[currentFrame_], imageIndex);

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo si(imageAvail_[currentFrame_], waitStage, cmdBufs_[currentFrame_],
                      renderDone_[imageIndex]);

    graphicsQueue_.submit(si, inFlight_[currentFrame_]);

    vk::PresentInfoKHR pi(renderDone_[imageIndex], swapchain_, imageIndex);

    vk::Result presentResult = presentQueue_.presentKHR(pi);
    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR || framebufferResized_)
    {
        framebufferResized_ = false;
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
    device_.waitIdle();
}

void GraphicsDriver::framebufferResized()
{
    framebufferResized_ = true;
}

} // namespace fire_engine
