# Fire Engine

A Vulkan-based 3D renderer written in C++23, built on macOS with MoltenVK.

After I came out of Uni, my first real job was working as a software engineer for [Rare](https://www.rare.co.uk).
I started there working on their R&D team, developing what would be called the 'REngine' - a shared 3D engine that was used be used for a bunch of future Gamecube releases (but not Perfect Dark Zero they did their own).
During those days I had a design for a 3D engine I'd create it Gamecube limitations wheren't a problem (tesselated surfaces, spline based animation and softbody skinning, and 'correct' collision detection).
I've no doubt these are all solved problems nowadays with the Unreal engine et all, but since I've been reading up on the tech again, I thought it would fun to dip my toes in again with Vulkan.

## Features

- **glTF 2.0 model loading** via [fastgltf](https://github.com/spnda/fastgltf) — geometry, PBR materials, textures, skeletal skins, and morph targets
- **Keyframe animation** — LINEAR interpolation with SLERP for quaternion rotation, plus translation, scale, and morph weight channels with looping playback
- **Skeletal skinning** — GPU joint matrix blending with up to 64 joints per skin
- **Morph target animation** — vertex position/normal deltas blended via weights in the vertex shader
- **Scenegraph architecture** — tree of Nodes with Component variants (Camera, Animator, Mesh, Empty) that propagate transforms, input, and draw commands
- **Backend-decoupled graphics layer** — graphics classes use opaque handles (`BufferHandle`, `TextureHandle`, `DescriptorSetHandle`) and emit `DrawCommand` structs with no Vulkan dependencies. All Vulkan code is confined to `render/`
- **Vulkan rendering** via vulkan.hpp C++ bindings with a 6-binding descriptor set layout (model/view/proj UBO, material UBO, texture sampler, skin UBO, morph UBO, morph targets SSBO)
- **Texture mapping** via [stb_image](https://github.com/nothings/stb), uploaded to GPU through staging buffers
- **First-person camera** with keyboard (WASD/QE) and mouse controls
- **GLSL shaders** compiled to SPIR-V at build time via `glslc`

## How It Works

### Scenegraph

The engine uses a scenegraph where each `Node` holds a `Transform` (position, rotation, scale) and an optional `Component`. Components are stored as a `std::variant<Empty, Animator, Camera, Mesh>`. Each component implements `update()` and `render()`:

- **Camera** processes input deltas to update position/yaw/pitch. FireEngine owns the active Camera directly and passes its position/target to the renderer each frame.
- **Animator** owns a `LinearAnimation` (rotation, translation, scale, and morph weight keyframes). Each frame it samples the animation at the current elapsed time, producing a TRS matrix that is applied to all child nodes.
- **Mesh** wraps an `Object` that manages GPU resources via opaque handles. During render traversal it calls `Object::render()` which writes UBO data to mapped memory and returns `DrawCommand` structs.
- **Empty** is a no-op component for structural nodes (joint bones, group nodes).

### Graphics/Render Boundary

The `graphics/` layer is fully decoupled from Vulkan:

- **Opaque handles** — `BufferHandle`, `TextureHandle`, `DescriptorSetHandle` are scoped enums backed by `uint32_t`. Graphics classes store these instead of Vulkan objects.
- **Resources** (in `render/`) — owns all Vulkan GPU resources and hands out opaque handles. Graphics classes call `Resources` methods during `load()` to create buffers, textures, and descriptor sets.
- **DrawCommand** — a backend-agnostic struct containing handle references and an index count. `Object::render()` returns a vector of these; the Renderer resolves handles to Vulkan objects and records the actual draw calls.
- **FrameInfo** — plain data struct carrying frame index, viewport dimensions, and camera vectors. Passed to graphics classes instead of the Vulkan-aware RenderContext.

### Loading

`GltfLoader` parses a glTF 2.0 file using fastgltf and builds the scenegraph. For each glTF node:

- If the node has an animation channel, it creates a three-level hierarchy: root Node (with the node's TRS transform) -> Animator Node (with keyframe data extracted from the glTF animation) -> Mesh Node.
- Otherwise, the node maps directly with its transform and mesh data.
- Skin data (joint references and inverse bind matrices) is loaded and attached to the relevant Mesh nodes.
- Morph target deltas (position and normal) are stored per-geometry and uploaded as SSBOs.

Animation keyframes (input times and output quaternions/vectors/weights) are read from glTF accessor data and set on the Animator's `LinearAnimation`.

### Frame Loop

1. `FireEngine::mainLoop()` polls GLFW input and calls `scene_.update(input_state)`
2. `SceneGraph::update()` propagates transforms and input down the node tree; each Node caches its `composedWorld` matrix for skin joint lookups
3. `Renderer::drawFrame()` acquires a swapchain image, begins the render pass, and calls `scene.render(ctx)`
4. During render traversal, Animator nodes apply their sampled TRS matrix to children's world transforms; Mesh nodes call `Object::render()` which writes UBOs and returns DrawCommands
5. The Renderer iterates collected DrawCommands, resolves opaque handles to Vulkan objects via Resources, and records bind/draw commands
6. The renderer ends the render pass, submits the command buffer, and presents

### Rendering Pipeline

- Descriptor set layout with 6 bindings:
  - Binding 0: model/view/projection + camera position UBO
  - Binding 1: material UBO (PBR properties)
  - Binding 2: texture sampler
  - Binding 3: skin UBO (joint matrices, `mat4[64]`)
  - Binding 4: morph UBO (morph metadata + weights)
  - Binding 5: morph targets SSBO (position/normal deltas)
- Resources class owns all GPU resources and exposes opaque handles
- Each Object creates its own descriptor pool and sets via Resources
- Depth buffering and swapchain recreation on window resize

### Vertex Shader Pipeline

1. **Morph targets** (if enabled): accumulates weighted position/normal deltas from the SSBO
2. **Skinning** (if enabled): blends joint matrices using per-vertex joint indices and weights
3. **Transform**: applies either the blended skin matrix or the model matrix to produce world-space position

## Setup

Setup [vcpkg](https://vcpkg.io/en/) on the build machine, and ensure that `VCPKG_ROOT` is available in the `PATH` environment variable.
Details of how to do this can be found at steps 1 and 2 in this [getting started doc](https://learn.microsoft.com/en-gb/vcpkg/get_started/get-started).
Ensure the `vcpkg` executable is available in your `PATH`.

Configure CMake, which will install and build dependencies via vcpkg.
Additionally, since I use `NeoVim`, I export the `compile_commands.json` to the build directory for use with `clangd`:

```bash
cmake --preset=vcpkg -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

Build:

```bash
cmake --build build
```

Run the tests:

```bash
./build/test_fire_engine
```

Run the application:

```bash
cd build && ./fireEngineApp
```

## Dependencies

Managed via vcpkg:

- `vulkan-headers` — Vulkan API headers
- `fastgltf` — glTF 2.0 parser
- `stb` — image loading (stb_image)
- `gtest` — Google Test framework

Also requires:

- System GLFW 3.3+
- Vulkan SDK (for `glslc` shader compiler)

## Assets

All assets used are from the Khronos glTF Sample Models [repository](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main).
