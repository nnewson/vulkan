# fire_engine

A Vulkan-based 3D renderer written in C++23, built on macOS with MoltenVK.

After I came out of Uni, my first real job was working as a software engineer for [Rare](https://www.rare.co.uk).
I started there working on their R&D team, developing what would be called the 'REngine' - a shared 3D engine that was used be used for a bunch of future Gamecube releases (but not Perfect Dark Zero they did their own).
During those days I had a design for a 3D engine I'd create it Gamecube limitations wheren't a problem (tesselated surfaces, spline based animation and softbody skinning, and 'correct' collision detection).
I've no doubt these are all solved problems nowadays with the Unreal engine et all, but since I've been reading up on the tech again, I thought it would fun to dip my toes in again with Vulkan.

## Features

- **glTF 2.0 model loading** via [fastgltf](https://github.com/spnda/fastgltf) — geometry, PBR materials, and textures
- **Keyframe animation** — LINEAR interpolation with SLERP for quaternion rotation, looping playback
- **Scenegraph architecture** — tree of Nodes with Component variants (Camera, Animator, Mesh) that propagate transforms, input, and draw commands
- **Vulkan rendering** via vulkan.hpp C++ bindings with configurable descriptor sets for model/view/projection UBOs, material UBOs, and texture samplers
- **Texture mapping** via [stb_image](https://github.com/nothings/stb), uploaded to GPU through staging buffers
- **First-person camera** with keyboard (WASD/QE) and mouse controls
- **GLSL shaders** compiled to SPIR-V at build time via `glslc`

## How It Works

### Scenegraph

The engine uses a scenegraph where each `Node` holds a `Transform` (position, rotation, scale) and an optional `Component`. Components are stored as a `std::variant<Animator, Camera, Mesh>`. Each component implements `update()` and `render()`:

- **Camera** processes input deltas to update position/yaw/pitch. FireEngine owns the active Camera directly and passes its position/target to the renderer each frame.
- **Animator** owns a `LinearAnimation` (a set of time/quaternion keyframes). Each frame it samples the animation at the current elapsed time using SLERP interpolation, producing a rotation matrix that is applied to all child nodes.
- **Mesh** owns its GPU resources (vertex/index buffers, texture, descriptor sets, UBO buffers). During render traversal it writes its UBO and records Vulkan draw commands directly on the active command buffer.

### Loading

`GltfLoader` parses a glTF 2.0 file using fastgltf and builds the scenegraph. For each glTF node:

- If the node has an animation channel, it creates a three-level hierarchy: root Node (with the node's TRS transform) → Animator Node (with keyframe data extracted from the glTF animation) → Mesh Node.
- Otherwise, the node maps directly with its transform and mesh data.

Animation keyframes (input times and output quaternions) are read from glTF accessor data and set on the Animator's `LinearAnimation`.

### Frame Loop

1. `FireEngine::mainLoop()` polls GLFW input and calls `scene_.update(input_state)`
2. `SceneGraph::update()` propagates transforms and input down the node tree
3. `Renderer::drawFrame()` acquires a swapchain image, begins the render pass, and calls `scene.render(ctx)`
4. During render traversal, Animator nodes apply their sampled rotation matrix to children's world transforms; Mesh nodes write their UBOs and record draw commands
5. The renderer ends the render pass, submits the command buffer, and presents

### Rendering Pipeline

- Descriptor set layout: binding 0 (model/view/projection + camera position UBO), binding 1 (material UBO), binding 2 (texture sampler)
- Each Mesh owns its own descriptor pool and sets, supporting multiple meshes in the scene
- Depth buffering and swapchain recreation on window resize

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
