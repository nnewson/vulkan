#include <fire_engine/renderer/renderer.hpp>

namespace fire_engine
{

Renderer::Renderer(const Window& window)
    : device_(window),
      swapchain_(device_, window),
      pipeline_(device_, swapchain_)
{
    swapchain_.createDepthResources(device_);
    swapchain_.createFramebuffers(pipeline_.renderPass());
}

} // namespace fire_engine
