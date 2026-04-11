# CLAUDE.md

## Project Overview

**fire_engine** is a Vulkan-based 3D renderer written in C++23. It loads glTF 2.0 models with texture mapping and PBR materials, and supports keyframe animation (LINEAR interpolation via SLERP). Built on macOS with MoltenVK. Uses a scenegraph architecture where components (Camera, Animator, Mesh) handle update and render logic.

## Build

```bash
cd build
cmake ..
cmake --build .
./fireEngineApp        # run the app (from build dir)
./test_fire_engine     # run all tests
```

Shaders are compiled from GLSL to SPIR-V via `glslc` as part of the build. Assets are copied to the build directory on every build via `cmake/copy_assets.cmake`.

## Dependencies

Managed via vcpkg: `vulkan-headers`, `gtest`, `stb`, `fastgltf`. Also requires system GLFW 3.3+ and the Vulkan SDK (for `glslc`).

## Project Structure

```
include/fire_engine/
  animation/       # LinearAnimation (keyframe sampling, SLERP)
  core/            # System (GLFW lifecycle), ShaderLoader, GltfLoader
  math/            # Vec3, Mat4, constants (header-only)
  graphics/        # Colour3, Vertex, Geometry, Material, Image, Texture
  input/           # CameraState, Input
  platform/        # Window, Keyboard, Mouse (Keyboard/Mouse header-only)
  render/          # Renderer, Device, Swapchain, Pipeline, Frame, RenderContext, UBO
  scene/           # Component, Node, SceneGraph, Transform, Camera, Animator, Mesh
  fire_engine.hpp
src/
  animation/       # linear_animation.cpp
  core/            # system.cpp, shader_loader.cpp, gltf_loader.cpp
  graphics/        # image.cpp, texture.cpp
  input/           # input.cpp
  platform/        # window.cpp, application.cpp (main entry point)
  render/          # device.cpp, frame.cpp, pipeline.cpp, renderer.cpp, swapchain.cpp
  scene/           # animator.cpp, camera.cpp, mesh.cpp, node.cpp, scene_graph.cpp, transform.cpp
  fire_engine.cpp
shaders/           # GLSL vertex/fragment shaders
tests/
  animation/       # test_linear_animation.cpp
  core/            # test_shader_loader.cpp
  math/            # test_vec3.cpp, test_mat4.cpp
  graphics/        # test_colour3.cpp, test_image.cpp, test_vertex.cpp
  input/           # test_camera_state.cpp
  scene/           # test_camera.cpp, test_node.cpp, test_scene_graph.cpp, test_transform.cpp
  assets/          # Minimal OBJ, MTL, PNG, BIN files for testing
assets/            # glTF models (AnimatedCube/) and fallback textures
cmake/             # Build-time scripts (copy_assets.cmake)
```

## Architecture

### Renderer Subsystem (`render/`)

- **Device** — wraps Vulkan instance, surface, physical/logical device, queues, `findMemoryType`, `createBuffer`
- **Swapchain** — swapchain, image views, depth resources, framebuffers; handles `cleanup()`/`recreate()`
- **Pipeline** — render pass, descriptor set layout, pipeline layout, graphics pipeline
- **Frame** — command pool, command buffers, sync objects (semaphores, fences). Pure frame orchestration; owns no per-mesh resources
- **Renderer** — owns Device, Swapchain, Pipeline, Frame. `drawFrame()` acquires a swapchain image, begins the render pass, binds the pipeline, calls `SceneGraph::render(ctx)` to let components record draw commands, then ends the render pass and submits
- **RenderContext** — per-frame data bag passed through the scenegraph during render: Device, Swapchain, Frame, Pipeline, active CommandBuffer, currentFrame index, camera position/target
- **UBO** (`ubo.hpp`) — shared `UniformBufferObject`, `MaterialUBO` structs, `MAX_FRAMES_IN_FLIGHT` constant

### Animation (`animation/`)

- **LinearAnimation** — stores rotation keyframes (time + quaternion) and samples them at a given time. Uses SLERP for interpolation between keyframes, loops automatically. Converts the interpolated quaternion directly to a rotation Mat4

### SceneGraph (`scene/`)

- **Component** — abstract base with `update(CameraState, Transform)` and `render(RenderContext, Mat4) -> Mat4`. The returned Mat4 is propagated to children as their parent world matrix
- **Node** — holds a name, Transform, a single `Components` variant, parent pointer, and children. `update()` and `render()` propagate through the tree via `std::visit`
- **SceneGraph** — owns root nodes. `update()` propagates input through the tree. `render()` propagates the RenderContext through the tree. Has no camera knowledge — FireEngine owns the active Camera directly
- **Transform** — position, rotation (Euler), scale → computes local and world matrices
- **Camera** — processes input deltas to update position/yaw/pitch. `render()` is a no-op (returns world unchanged)
- **Animator** — owns a `LinearAnimation`, samples it each frame in `update()`, returns `world * modelMatrix_` from `render()` so children inherit the animation
- **Mesh** — owns all its GPU resources: vertex/index buffers, texture, material UBOs, UBO buffers, descriptor pool/sets. `load(Geometry, Material, texturePath, Device, Pipeline, Frame)` creates everything. `render()` writes the UBO and records draw commands (bind buffers, bind descriptors, drawIndexed) directly on the RenderContext's command buffer

### Data Flow Per Frame

1. `FireEngine::mainLoop()` polls input, calls `scene_.update(input_state)`, then `renderer_->drawFrame(*window_, scene_, cameraPosition, cameraTarget)`
2. `SceneGraph::update()` propagates transforms and input through nodes; FireEngine reads camera position/target directly from its owned Camera pointer
3. `Renderer::drawFrame()` acquires image, begins render pass, builds RenderContext (with camera data), calls `scene.render(ctx)`
4. During render traversal: Animator applies its model matrix to children's world; Mesh writes its UBO and records its own draw commands
5. Renderer ends render pass, submits, presents

## Code Style

Formatting is enforced by `.clang-format` (Allman braces, 4-space indent, 100-column limit, left-aligned pointers, no single-line functions, constructor initializers each on their own line). Always write code that matches these rules.

- **C++23** standard throughout
- **`constexpr`** wherever possible, especially on math types
- **`[[nodiscard]]`** on all getters and pure functions
- **`noexcept`** on operations that cannot throw
- Non-explicit constructors on value types (Vec3, Colour3, Mat4) to allow brace-init
- Private members use trailing underscore: `x_`, `width_`
- Getters/setters share the same name: `float x() const` / `void x(float)`
- Static factory methods for file loading: `Class::load_from_file(path)`
- Compound assignment as primitives (`+=` modifies directly), binary operators delegate to them
- Math constants in `math/constants.hpp` (`pi`, `deg_to_rad`, `rad_to_deg`, `float_epsilon`)
- glTF loading via fastgltf (`GltfLoader::loadScene`) — extracts geometry, materials, textures, and animation keyframes
- Shader loading via `core/ShaderLoader::load_from_file(path)`
- Explicit rule-of-five on all classes (defaulted or deleted as appropriate)

### Class Template (Image as reference)

New classes should follow the structure of `Image` (`include/fire_engine/graphics/image.hpp`):

```cpp
class ClassName
{
public:
    // Static factory for file loading (if applicable)
    static ClassName load_from_file(const std::string& path);

    // Default constructor
    ClassName() = default;
    ~ClassName() = default;

    // Rule-of-five: explicit copy/move (defaulted or deleted)
    ClassName(const ClassName&) = default;
    ClassName& operator=(const ClassName&) = default;
    ClassName(ClassName&&) noexcept = default;
    ClassName& operator=(ClassName&&) noexcept = default;

    // Getters: [[nodiscard]], noexcept, same name as member without underscore
    [[nodiscard]] int width() const noexcept
    {
        return width_;
    }

    // Setters: same name as getter, take value by appropriate type
    void width(int w) noexcept
    {
        width_ = w;
    }

private:
    // Members: trailing underscore, in-class default initialisers
    int width_{0};
};
```

Key points:
- All members private with trailing underscore and default initialisers
- Getter/setter pairs share the same name (overloaded)
- `[[nodiscard]]` and `noexcept` on all getters
- `noexcept` on setters that cannot throw
- Rule-of-five always explicit, even if all defaulted
- Static `load_from_file` factory for any class that loads from disk
- Non-instantiable utility classes use `ClassName() = delete` (e.g. ShaderLoader)

## Testing

Google Test framework. All tests in a single executable `test_fire_engine`. Test files mirror the source structure: `tests/animation/`, `tests/math/`, `tests/graphics/`, `tests/scene/`, etc. Test assets (minimal OBJ, MTL, PNG files) live in `tests/assets/` and are copied to `build/test_assets/` at configure time. Tests cover construction, accessors, equality, arithmetic, file loading, move/copy semantics, animation sampling/interpolation, edge cases, constexpr validation, and noexcept guarantees.

## Rendering Pipeline

- Vulkan with vulkan.hpp (C++ bindings)
- Descriptor set layout: binding 0 (UBO: model/view/proj/cameraPos), binding 1 (MaterialUBO), binding 2 (texture sampler)
- Each Mesh owns its own descriptor pool/sets and UBO buffers — supports multiple meshes
- glTF 2.0 loading via fastgltf — geometry, PBR materials, textures, and keyframe animations
- Texture loading via stb_image (RGBA), uploaded to GPU via staging buffer
- GLFW for windowing with keyboard (WASD/QE) and mouse camera controls
