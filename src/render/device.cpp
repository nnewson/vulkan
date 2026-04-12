#include <iostream>
#include <set>
#include <string>

#include <fire_engine/render/device.hpp>

namespace fire_engine
{

#ifdef NDEBUG
constexpr bool enableValidation = false;
#else
constexpr bool enableValidation = true;
#endif

static const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
static const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset", // required on macOS/MoltenVK
};

Device::Device(const Window& window)
{
    createInstance();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
}

void Device::createInstance()
{
    constexpr vk::ApplicationInfo appInfo("FireEngine", VK_MAKE_VERSION(1, 0, 0), "No Engine",
                                          VK_MAKE_VERSION(1, 0, 0), vk::ApiVersion14);

    auto exts = Window::requiredVulkanExtensions();
    exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    exts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    std::vector<const char*> layers;
    if (enableValidation)
    {
        layers = validationLayers;
        printValidationInfo();
    }

    vk::InstanceCreateInfo ci(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &appInfo,
                              layers, exts);

    instance_ = vk::raii::Instance(context_, ci);
}

void Device::printValidationInfo() const
{
    auto layersAvailable = context_.enumerateInstanceLayerProperties();
    std::cout << "Available layers:\n";
    for (const auto& layer : layersAvailable)
    {
        std::cout << '\t' << layer.layerName << '\n';
    }
    std::cout << '\n';

    auto extensions = context_.enumerateInstanceExtensionProperties();
    std::cout << "Available extensions:\n";
    for (const auto& ext : extensions)
    {
        std::cout << '\t' << ext.extensionName << '\n';
    }
    std::cout << '\n';
}

void Device::createSurface(const Window& window)
{
    surface_ = window.createVulkanSurface(instance_);
}

void Device::pickPhysicalDevice()
{
    auto devs = instance_.enumeratePhysicalDevices();
    for (auto& d : devs)
    {
        if (isDeviceSuitable(d))
        {
            physDevice_ = std::move(d);
            return;
        }
    }
    throw std::runtime_error("no suitable GPU found");
}

bool Device::isDeviceSuitable(const vk::raii::PhysicalDevice& d)
{
    auto [gf, pf] = findQueueFamilies(d);
    if (!gf.has_value() || !pf.has_value())
    {
        return false;
    }

    auto avail = d.enumerateDeviceExtensionProperties();
    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
    for (auto& e : avail)
    {
        required.erase(e.extensionName);
    }
    if (!required.empty())
    {
        return false;
    }

    auto fmts = d.getSurfaceFormatsKHR(*surface_);
    auto modes = d.getSurfacePresentModesKHR(*surface_);
    return !fmts.empty() && !modes.empty();
}

std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
Device::findQueueFamilies(const vk::raii::PhysicalDevice& d)
{
    auto families = d.getQueueFamilyProperties();
    std::optional<uint32_t> gf, pf;
    for (uint32_t i = 0; i < families.size(); ++i)
    {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            gf = i;
        }
        if (d.getSurfaceSupportKHR(i, *surface_))
        {
            pf = i;
        }
        if (gf && pf)
        {
            break;
        }
    }
    return {gf, pf};
}

void Device::createLogicalDevice()
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

    device_ = vk::raii::Device(physDevice_, ci);
    graphicsQueue_ = device_.getQueue(graphicsFamily_, 0);
    presentQueue_ = device_.getQueue(presentFamily_, 0);
}

uint32_t Device::findMemoryType(uint32_t filter, vk::MemoryPropertyFlags props) const
{
    auto mem = physDevice_.getMemoryProperties();
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
    {
        if ((filter & (1 << i)) && (mem.memoryTypes[i].propertyFlags & props) == props)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
Device::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                     vk::MemoryPropertyFlags props) const
{
    vk::BufferCreateInfo ci({}, size, usage, vk::SharingMode::eExclusive);
    vk::raii::Buffer buf(device_, ci);

    auto req = buf.getMemoryRequirements();
    vk::MemoryAllocateInfo ai(req.size, findMemoryType(req.memoryTypeBits, props));
    vk::raii::DeviceMemory mem(device_, ai);
    buf.bindMemory(*mem, 0);

    return {std::move(buf), std::move(mem)};
}

} // namespace fire_engine
