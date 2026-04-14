# CLAUDE.md

## Project Overview

**fire_engine** is a Vulkan-based 3D renderer written in C++23. It loads glTF 2.0 models with PBR materials, texture mapping, skeletal skinning (GPU joint matrix blending), and morph target vertex animation. Keyframe animation supports rotation (SLERP), translation, scale, and morph weight channels with LINEAR interpolation. Built on macOS with MoltenVK. Uses a scenegraph architecture where components (Camera, Animator, Mesh, Empty) handle update and render logic.

The graphics layer (`graphics/`) is fully decoupled from Vulkan — it uses opaque handles and emits backend-agnostic `DrawCommand` structs. All Vulkan-specific code lives in the render layer (`render/`), with `Resources` acting as the bridge that owns GPU resources and hands out opaque handles.

## Build

```bash
cd build
cmake ..
cmake --build .
./fireEngineApp        # run the app (from build dir)
./test_fire_engine     # run all tests (525 tests)
```

Shaders are compiled from GLSL to SPIR-V via `glslc` as part of the build. Assets are copied to the build directory on every build via `cmake/copy_assets.cmake`.

## Dependencies

Managed via vcpkg: `vulkan-headers`, `gtest`, `stb`, `fastgltf`. Also requires system GLFW 3.3+ and the Vulkan SDK (for `glslc`).

## Project Structure

```
include/fire_engine/
  animation/       # LinearAnimation (rotation/translation/scale/weight keyframes, SLERP)
  core/            # System (GLFW lifecycle), ShaderLoader, GltfLoader
  math/            # Vec3, Mat4, Quaternion, constants (header-only)
  graphics/        # Colour3, Vertex, Geometry, Material, Image, Texture, Object, Assets, Skin,
                   # DrawCommand, FrameInfo, gpu_handle (opaque handle types)
  input/           # CameraState, Input
  platform/        # Window, Keyboard, Mouse (Keyboard/Mouse header-only)
  render/          # Renderer, Device, Swapchain, Pipeline, Frame, RenderContext, Resources, UBO, constants
  scene/           # Component, Components, Node, SceneGraph, Transform, Camera, Animator, Mesh, Empty
  fire_engine.hpp
src/
  animation/       # linear_animation.cpp
  core/            # system.cpp, shader_loader.cpp, gltf_loader.cpp
  graphics/        # image.cpp, texture.cpp, geometry.cpp, skin.cpp, object.cpp
  input/           # input.cpp
  platform/        # window.cpp, application.cpp (main entry point)
  render/          # device.cpp, frame.cpp, pipeline.cpp, render_context.cpp, renderer.cpp, resources.cpp, swapchain.cpp
  scene/           # animator.cpp, camera.cpp, mesh.cpp, node.cpp, scene_graph.cpp, transform.cpp, empty.cpp
  fire_engine.cpp
shaders/           # GLSL vertex/fragment shaders
tests/
  animation/       # test_linear_animation.cpp
  core/            # test_shader_loader.cpp, test_gltf_loader.cpp, test_system.cpp
  math/            # test_vec3.cpp, test_mat4.cpp, test_quaternion.cpp
  graphics/        # test_colour3.cpp, test_image.cpp, test_vertex.cpp, test_material.cpp, test_geometry.cpp,
                   # test_skin.cpp, test_assets.cpp, test_gpu_handle.cpp, test_draw_command.cpp,
                   # test_frame_info.cpp, test_texture.cpp
  input/           # test_camera_state.cpp
  render/          # test_ubo.cpp
  scene/           # test_camera.cpp, test_node.cpp, test_scene_graph.cpp, test_transform.cpp, test_formatter.cpp, test_animator.cpp
  assets/          # Minimal OBJ, MTL, PNG, BIN files for testing
assets/            # glTF models (AnimatedCube/, BoxAnimated/, RiggedSimple/, AnimatedMorphCube/) and fallback textures
cmake/             # Build-time scripts (copy_assets.cmake)
```

## Architecture

### Graphics/Render Boundary

The `graphics/` layer contains no Vulkan headers or `vk::` types. It communicates with the GPU through:

- **Opaque handles** (`gpu_handle.hpp`) — `BufferHandle`, `TextureHandle`, `DescriptorSetHandle` are scoped enums backed by `uint32_t`. `MappedMemory` is a `void*` alias for mapped GPU memory pointers.
- **DrawCommand** (`draw_command.hpp`) — backend-agnostic struct emitted by `Object::render()`. Contains handle references and an index count.
- **FrameInfo** (`frame_info.hpp`) — plain data struct carrying currentFrame, viewport dimensions, and camera vectors. Replaces direct RenderContext access in graphics/ code.
- **Resources** (`render/resources.hpp`) — the bridge class. Owns all Vulkan GPU resources (buffers, textures, descriptor pools/sets). Graphics classes call `Resources` methods during `load()` and receive opaque handles back. Renderer resolves handles to Vulkan objects during command recording.

### Renderer Subsystem (`render/`)

- **Device** — wraps Vulkan instance, surface, physical/logical device, queues, `findMemoryType`, `createBuffer`
- **Swapchain** — swapchain, image views, depth resources, framebuffers; handles `cleanup()`/`recreate()`
- **Pipeline** — render pass, descriptor set layout (6 bindings), pipeline layout, graphics pipeline. Uses `offsetof(Vertex, ...)` via `friend` access to build vertex input descriptions
- **Frame** — command pool, command buffers, sync objects (semaphores, fences). Pure frame orchestration; owns no per-mesh resources
- **Resources** — owns all Vulkan GPU resources on behalf of graphics objects. Creates vertex/index buffers, textures (with staging upload), mapped uniform/storage buffers, and descriptor pool/sets. Returns opaque handles. Provides Vulkan-typed accessors (`vulkanBuffer()`, `vulkanDescriptorSet()`) for Renderer command recording
- **Renderer** — owns Device, Swapchain, Pipeline, Frame, Resources. `drawFrame()` acquires a swapchain image, begins the render pass, binds the pipeline, calls `SceneGraph::render(ctx)` to collect `DrawCommand` structs, then iterates them to record Vulkan bind/draw calls, ends the render pass and submits
- **RenderContext** — per-frame data bag passed through the scenegraph during render: Device, Swapchain, Frame, Pipeline, active CommandBuffer, currentFrame index, camera position/target, `DrawCommand` collection pointer. Provides `frameInfo()` to extract a Vulkan-free `FrameInfo` for graphics/ classes
- **UBO** (`ubo.hpp`) — shared UBO structs:
  - `UniformBufferObject` — model/view/proj matrices, cameraPos, `hasSkin` flag
  - `MaterialUBO` — PBR material properties
  - `SkinUBO` — `Mat4 joints[MAX_JOINTS]` for skeletal animation
  - `MorphUBO` — `hasMorph`, `morphTargetCount`, `vertexCount`, `float weights[MAX_MORPH_TARGETS]`
- **constants** (`constants.hpp`) — `MAX_FRAMES_IN_FLIGHT=2`, `MAX_JOINTS=64`, `MAX_MORPH_TARGETS=8`

### Graphics (`graphics/`)

No Vulkan headers or `vk::` types in any file.

- **Object** — manages per-geometry rendering state via opaque handles and mapped memory pointers. Inner `GeometryBindings` struct groups per-geometry `MappedMemory` arrays and `DescriptorSetHandle` arrays. `load(Resources&)` creates all GPU resources. `render(FrameInfo, Mat4)` writes UBO data via memcpy to mapped pointers and returns a `vector<DrawCommand>`
- **Assets** — centralized manager holding vectors of Texture, Material, Geometry, Skin, and LinearAnimation. GltfLoader populates it during scene loading; components reference assets by index
- **Skin** — holds joint Node pointers and inverse bind matrices. `updateJointMatrices()` computes final joint matrices as `inverseBindMatrix * node.composedWorld()`. `computeJointMatrices()` returns a fresh vector; `cachedJointMatrices()` returns the last computed result
- **Geometry** — vertices, indices, material pointer; holds `BufferHandle` for vertex/index buffers after `load(Resources&)`. Also stores morph target deltas: `morphPositions_` and `morphNormals_` (vector of vectors of Vec3, one per target)
- **Vertex** — 6 vertex attributes: position (vec3), colour (vec3), normal (vec3), texCoord (vec2), joints (uvec4), weights (vec4). The joints/weights attributes support skeletal skinning. Pipeline accesses member offsets via `friend class Pipeline`
- **Texture** — holds a single `TextureHandle`. Factory methods `load_from_file(path, Resources&)` and `load_from_data(pixels, w, h, Resources&)` delegate to Resources. `loaded()` checks handle against `NullTexture`
- **Material** — PBR material properties (ambient, diffuse, specular, emissive, roughness, metallic, etc.)
- **Image** — CPU-side image data (pixels, width, height, channels)
- **Colour3** — RGB colour value type

### Animation (`animation/`)

- **LinearAnimation** — stores 4 keyframe channel types:
  - `RotationKeyframe` — time + quaternion (qx, qy, qz, qw)
  - `TranslationKeyframe` — time + Vec3 position
  - `ScaleKeyframe` — time + Vec3 scale
  - `WeightKeyframe` — time + vector of float weights (for morph targets)
- `sample(float t)` returns a combined TRS Mat4 using SLERP for rotation, LERP for translation/scale
- `sampleWeights(float t, numTargets)` interpolates morph target weights
- `duration()` can be explicitly overridden so all channels in a glTF animation loop in lockstep

### SceneGraph (`scene/`)

- **Component** — abstract base with `update(CameraState, Transform)` and `render(RenderContext, Mat4) -> Mat4`. The returned Mat4 is propagated to children as their parent world matrix
- **Components** — `std::variant<Empty, Animator, Camera, Mesh>` with `componentName()` helper
- **Node** — holds a name, Transform, a single Components variant, parent pointer, children, and a cached `composedWorld_` matrix (used by Skin for joint lookups). `update()` takes `parentComposedWorld` and propagates through the tree. Has a `std::formatter` specialization for debug printing
- **SceneGraph** — owns root nodes. `update()` propagates input through the tree. `render()` propagates the RenderContext through the tree. Has no camera knowledge — FireEngine owns the active Camera directly
- **Transform** — position, rotation (Euler), scale → computes local and world matrices. Has a `std::formatter` specialization
- **Camera** — processes input deltas to update position/yaw/pitch. `render()` is a no-op (returns world unchanged)
- **Animator** — owns a `LinearAnimation`, samples it each frame in `update()`, returns `world * modelMatrix_` from `render()` so children inherit the animation
- **Mesh** — wraps an `Object` (does not own GPU resources directly). Supports optional skin pointer, morph animation pointer, and initial morph weights. `update()` samples morph weights from animation if present. `render()` calls `Object::render(ctx.frameInfo(), world)` and pushes returned DrawCommands to `ctx.drawCommands`
- **Empty** — no-op component for structural nodes (joint bones, group nodes)

### Data Flow Per Frame

1. `FireEngine::mainLoop()` polls input, calls `scene_.update(input_state)`, then `renderer_->drawFrame(*window_, scene_, cameraPosition, cameraTarget)`
2. `SceneGraph::update()` propagates transforms and input through nodes; each Node caches its `composedWorld_` for skin joint lookups. FireEngine reads camera position/target directly from its owned Camera pointer
3. `Renderer::drawFrame()` acquires image, begins render pass, builds RenderContext (with camera data and DrawCommand collection pointer), calls `scene.render(ctx)`
4. During render traversal:
   - Skin::updateJointMatrices() uses each joint Node's `composedWorld()` to compute final joint matrices
   - Mesh::update() samples morph weights from its animation (if present)
   - Object::render() writes all UBOs (main + skin + morph) via memcpy to mapped pointers, returns `vector<DrawCommand>`
   - Mesh::render() pushes DrawCommands to the RenderContext collection
   - Animator applies its sampled TRS matrix to children's world
5. Renderer iterates collected DrawCommands, resolves handles to Vulkan objects via Resources, records bind/draw commands
6. Renderer ends render pass, submits, presents

### Vertex Shader Pipeline

The vertex shader processes vertices in this order:
1. **Morph targets** (if `hasMorph==1`): accumulates weighted position/normal deltas from the MorphTargets SSBO using weights from MorphUBO
2. **Skinning** (if `hasSkin==1`): blends joint matrices from SkinUBO using per-vertex joint indices and weights
3. **Transform**: applies either the blended skin matrix or the model matrix to produce world-space position

## Code Style

Formatting is enforced by `.clang-format` (Allman braces, 4-space indent, 100-column limit, left-aligned pointers, no single-line functions, constructor initializers each on their own line). Always write code that matches these rules.

- **C++23** standard throughout
- **`constexpr`** wherever possible, especially on math types (Vec3, Mat4, Quaternion, Colour3)
- **`[[nodiscard]]`** on all getters and pure functions
- **`noexcept`** on operations that cannot throw
- Non-explicit constructors on value types (Vec3, Colour3, Mat4, Quaternion) to allow brace-init
- Private members use trailing underscore: `x_`, `width_`
- Getters/setters share the same name: `float x() const` / `void x(float)`
- Static factory methods for file loading: `Class::load_from_file(path)`
- Compound assignment as primitives (`+=` modifies directly), binary operators delegate to them
- Math constants in `math/constants.hpp` (`pi`, `deg_to_rad`, `rad_to_deg`, `float_epsilon`)
- glTF loading via fastgltf (`GltfLoader::loadScene`) — extracts geometry, materials, textures, skins, morph targets, and keyframe animations
- Shader loading via `core/ShaderLoader::load_from_file(path)`
- Explicit rule-of-five on all classes (defaulted or deleted as appropriate)
- `std::formatter` specializations for debug output (Node, Transform)
- Declarations from a `.hpp` always go in a matching `.cpp` file, never in an unrelated `.cpp`

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

Google Test framework. 525 tests in a single executable `test_fire_engine`. Test files mirror the source structure: `tests/animation/`, `tests/math/`, `tests/graphics/`, `tests/render/`, `tests/scene/`, etc. Test assets (minimal OBJ, MTL, PNG files) live in `tests/assets/` and are copied to `build/test_assets/` at configure time. Tests cover construction, accessors, equality, arithmetic, file loading, move/copy semantics, animation sampling/interpolation, skinning, morph targets, UBO layout, opaque handle types, DrawCommand/FrameInfo structs, edge cases, constexpr validation, and noexcept guarantees. The graphics layer tests run without a GPU since all Vulkan types are abstracted behind opaque handles.

## Rendering Pipeline

- Vulkan with vulkan.hpp (C++ bindings), confined to `render/` and `shaders/`
- Descriptor set layout with 6 bindings:
  - Binding 0: UBO (model/view/proj/cameraPos/hasSkin)
  - Binding 1: MaterialUBO (PBR properties)
  - Binding 2: Texture sampler
  - Binding 3: SkinUBO (joint matrices — `mat4[64]`)
  - Binding 4: MorphUBO (morph metadata + weights as `vec4[2]`)
  - Binding 5: MorphTargets SSBO (std430, readonly — position/normal deltas)
- Resources class owns all GPU resources (buffers, textures, descriptor pools/sets) and exposes opaque handles to graphics/ classes
- Object emits `DrawCommand` structs; Renderer resolves handles and records Vulkan bind/draw calls
- glTF 2.0 loading via fastgltf — geometry, PBR materials, textures, skins, morph targets, and keyframe animations
- Texture loading via stb_image (RGBA), uploaded to GPU via staging buffer
- GLFW for windowing with keyboard (WASD/QE) and mouse camera controls
