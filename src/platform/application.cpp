// Vulkan rotating cube — matches shader.vert / shader.frag in this repo.
// Dependencies: Vulkan SDK, GLFW 3.3+, glslc (for SPIR-V compilation via CMake).

#include <fire_engine/fire_engine.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace fire_engine;

int main(int argc, char* argv[])
{
    std::string_view scene_path = (argc >= 2) ? argv[1] : "";
    std::string_view skybox_path = (argc >= 3) ? argv[2] : "";
    try
    {
        FireEngine app;
        app.run(800, 600, "FireEngine Demo", scene_path, skybox_path);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
