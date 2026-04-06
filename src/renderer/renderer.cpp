#include <fire_engine/renderer/renderer.hpp>

namespace fire_engine
{

Renderer::Renderer(const Window& window)
    : device_(window)
    , swapchain_(device_, window)
{
}

} // namespace fire_engine
