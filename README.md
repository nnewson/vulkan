# Fire Engine

A Vulkan-based 3D renderer written in C++23, built on macOS with MoltenVK.

After I came out of Uni, my first real job was working as a software engineer for [Rare](https://www.rare.co.uk).
I started there working on their R&D team, developing what would be called the 'REngine' - a shared 3D engine that was used be used for a bunch of future Gamecube releases (but not Perfect Dark Zero they did their own).
During those days I had a design for a 3D engine I'd create it Gamecube limitations wheren't a problem (tesselated surfaces, spline based animation and softbody skinning, and 'correct' collision detection).
I've no doubt these are all solved problems nowadays with the Unreal engine et all, but since I've been reading up on the tech again, I thought it would fun to dip my toes in again with Vulkan.

## Features

- **glTF 2.0 model loading** via [fastgltf](https://github.com/spnda/fastgltf) — geometry, full PBR material set (base-colour, metallic-roughness, normal, occlusion, emissive), per-texture sampler settings, skeletal skins, morph targets, and alpha-mode state (OPAQUE / MASK / BLEND, `alphaCutoff`, `doubleSided`)
- **Tangent-space normal mapping** — tangents generated on load when a material uses a normal texture (per-triangle UV derivatives, Gram-Schmidt orthogonalisation, handedness preserved in `tangent.w`)
- **Physically based shading with split-sum IBL + multi-scatter compensation** — equirectangular HDR skybox is converted to an environment cubemap (1024², 11 mip levels), a diffuse irradiance cubemap (32²), a GGX prefiltered specular cubemap (128², 8 mips, importance-sampled with 256 Hammersley samples and Filament-style mip-LOD weighting against the source cubemap's mip chain), and a BRDF integration LUT (256²) at startup. Forward fragment shader uses Fdez-Aguera multi-scatter compensation so rough conductors stay energy-conserving across the roughness range
- **Directional light with cascaded shadow maps** — 4 cascades, 2048×2048 per cascade in a 2D-array depth image, log-uniform splits over 0.1m–50m, `sampler2DArrayShadow` with hardware 3×3 PCF, per-cascade bias scaling, 10% blend bands at boundaries to hide cascade seams. Optional debug tint (`renderer.cpp::cascadeDebugTint_`) colour-codes cascade regions for verification
- **HDR offscreen forward pass + ACES post-process** — forward renders into an R32G32B32A32 target, then a fullscreen post-process pass applies ACES tonemapping + gamma 2.2 before presenting to the swapchain
- **Keyframe animation** with per-channel interpolation (LINEAR with SLERP for quaternions, STEP, CUBICSPLINE with in/out tangents) across rotation, translation, scale, and morph weight channels; looping playback; runtime animation selection via `AnimationState`
- **Skeletal skinning** — GPU joint matrix blending, up to 64 joints per skin
- **Morph target animation** — vertex position/normal deltas uploaded as an SSBO, blended by weights in the vertex shader (up to 8 targets per mesh)
- **Alpha blending and masking** — three forward pipeline variants (opaque, double-sided opaque, blend) dispatched per draw from the material's `AlphaMode`/`doubleSided` flags. MASK handled via a fragment-shader discard test; BLEND uses straight-alpha blending with depth-write disabled and back-to-front sort of translucent draws
- **Scenegraph architecture** — tree of Nodes with Component variants (Camera, Animator, Mesh, Empty) that propagate transforms, an `InputState` bundle, and draw commands. Node transforms store rotation as a quaternion so orientations from glTF round-trip exactly
- **Backend-decoupled graphics layer** — graphics classes use opaque handles (`BufferHandle`, `TextureHandle`, `DescriptorSetHandle`, `PipelineHandle`) and emit `DrawCommand` structs with no Vulkan dependencies. IBL cubemaps, BRDF LUT, and shadow map are all owned by the render layer and referenced through the same handle types
- **Vulkan rendering** via vulkan.hpp C++ bindings with a 15-binding forward descriptor set, plus separate descriptor layouts for skybox, shadow, and post-process passes
- **Texture mapping** via [stb_image](https://github.com/nothings/stb), including HDR equirectangular loading for the skybox; uploaded to GPU through staging buffers
- **First-person camera** with keyboard (WASD + E/F for vertical) and mouse controls
- **GLSL shaders** compiled to SPIR-V at build time via `glslc`

## How It Works

### Scenegraph

The engine uses a scenegraph where each `Node` holds a `Transform` (position, unit-quaternion rotation, scale) and an optional `Component`. Components are stored as a `std::variant<Empty, Animator, Camera, Mesh>`. Each component implements `update(InputState, ...)` and `render(...)`:

- **Camera** integrates per-frame deltas from `InputState::cameraState()` (deltaPosition, deltaYaw, deltaPitch, deltaZoom) into absolute position and orientation. Pitch is clamped to ±1.5 rad. FireEngine owns the active Camera directly and passes its position/target to the renderer each frame.
- **Animator** owns an `Animation` (per-channel Linear/Step/CubicSpline interpolation across rotation, translation, scale, and morph weight keyframes). Each frame it samples the animation at the current elapsed time, producing a TRS matrix that is applied to all child nodes. It also consults `InputState::animationState()` to switch between animations at runtime.
- **Mesh** wraps an `Object` that manages GPU resources via opaque handles. During render traversal it calls `Object::render()` which writes UBO data to mapped memory and returns `DrawCommand` structs.
- **Empty** is a no-op component for structural nodes (joint bones, group nodes).

### Graphics/Render Boundary

The `graphics/` layer is fully decoupled from Vulkan:

- **Opaque handles** — `BufferHandle`, `TextureHandle`, `DescriptorSetHandle`, and `PipelineHandle` are scoped enums backed by `uint32_t`. Graphics classes store these instead of Vulkan objects. The IBL cubemaps, BRDF LUT, and shadow map live behind the same handle types.
- **Resources** (in `render/`) — owns all Vulkan GPU resources (buffers, textures, descriptor pools/sets, registered pipelines, IBL cubemaps, BRDF LUT, shadow map). Graphics classes call `Resources` methods during `load()` to create buffers, textures, and descriptor sets, and receive handles back.
- **DrawCommand** — a backend-agnostic struct containing handle references (including the `PipelineHandle` to bind), an index count, and a `sortDepth` used for back-to-front ordering of translucent draws. `Object::render()` returns a vector of these; the Renderer resolves handles to Vulkan objects and records the actual draw calls.
- **FrameInfo** — plain data struct carrying frame index, viewport dimensions, camera vectors, and the `AlphaPipelines` bundle so graphics code can pick the right pipeline per material without touching Vulkan types.

### Loading

`GltfLoader` parses a glTF 2.0 file using fastgltf and builds the scenegraph. For each glTF node:

- If the node has an animation channel, it creates a three-level hierarchy: root Node (with the node's TRS transform) -> Animator Node (with keyframe data, preserving each channel's interpolation mode) -> Mesh Node.
- Otherwise, the node maps directly with its transform and mesh data.
- Skin data (joint references and inverse bind matrices) is loaded and attached to the relevant Mesh nodes.
- Morph target deltas (position and normal) are stored per-geometry and uploaded as an SSBO.
- Materials gather up to five textures (base-colour, emissive, normal, metallic-roughness, occlusion), each with its own `SamplerSettings` and a `TextureEncoding` (Srgb or Linear) extracted from the glTF sampler definition.
- **Tangent generation** runs automatically when a material has a normal texture and the glTF did not already supply TANGENT data. A custom per-triangle routine computes T and B from UV derivatives, Gram-Schmidts T against the vertex normal, and writes handedness into `tangent.w`. Degenerate UVs fall back to a normal-derived tangent so the mesh still shades reasonably.

Animation keyframes (input times and output quaternions/vectors/weights, plus CUBICSPLINE tangents) are read from glTF accessor data and set on the Animator's `Animation`.

### Startup: IBL Precompute

Before the first frame, the renderer runs a one-shot precompute chain using transient pipelines and one-time-submit command buffers:

1. **Equirectangular → cubemap** — load the HDR skybox and render 6 cubemap faces into a 1024² RGBA32F cubemap by sampling the equirectangular map along per-face direction vectors. After the 6-face pass, a `vkCmdBlitImage` chain generates the full 11-mip pyramid (1024 → 1) so the prefilter pass can do mip-weighted importance sampling.
2. **Irradiance convolution** — produce a 32² RGBA32F cubemap via cosine-weighted hemisphere integration (diffuse IBL).
3. **GGX specular prefilter** — produce a 128² RGBA32F cubemap with 8 mip levels. Roughness is pushed as a push constant per mip; each fragment importance-samples GGX with Hammersley + 256 samples, picking a blurrier source-cubemap mip when the PDF is low (Filament's mip-weighted importance sampling) so rough lobes stay shimmer-free.
4. **BRDF integration LUT** — 256² RGBA32F 2D; x = NdotV, y = roughness; outputs (scale, bias) for the Fresnel split-sum. No input textures required.

The transient pipelines are destroyed once the bake completes; only the resulting cubemaps + LUT remain and are bound into the forward shader's descriptor set every frame.

### Frame Loop

1. `FireEngine::mainLoop()` polls GLFW, calls `input_.update(window, dt)` to produce an `InputState`, then `scene_.update(inputState)`
2. `SceneGraph::update()` propagates `InputState` and transforms down the node tree; each Node caches its `composedWorld` matrix for skin joint lookups
3. `Renderer::drawFrame()` acquires a swapchain image and records three sub-passes:
   - **Shadow pass** — looped 4 times, once per cascade. Each iteration begins a depth-only pass on its own framebuffer (one shadow-map array layer), pushes the cascade index as a push constant, and replays all collected shadow `DrawCommand`s. `shadow.vert` projects with `lightViewProj[cascadeIndex]` from the per-draw `ShadowUBO`. Skin and morph still apply
   - **Forward pass** — begin the HDR offscreen pass, draw the skybox (LEQUAL depth, no write), then call `scene.render(ctx)`; Mesh/Object emit `DrawCommand`s that the Renderer buckets into opaque vs blend, sorts the blend bucket back-to-front by `sortDepth`, and replays through the same bind/draw loop resolving handles via `Resources`
   - **Post-process pass** — begin the swapchain-format pass, draw a fullscreen triangle that samples the HDR target and applies ACES + gamma 2.2
4. Renderer submits the command buffer and presents

### Rendering Pipeline

- Forward descriptor set with 15 bindings:
  - 0 model/view/projection + camera position UBO
  - 1 Material UBO (`diffuseAlpha`, `emissiveRoughness`, `materialParams` for `normalScale` etc., `textureFlags`, `extraFlags`)
  - 2 base-colour sampler
  - 3 skin UBO (joint matrices, `mat4[64]`)
  - 4 morph UBO (metadata + weights)
  - 5 morph targets SSBO (position/normal deltas as `vec4[]`)
  - 6 emissive sampler
  - 7 normal sampler
  - 8 metallic-roughness sampler
  - 9 occlusion sampler
  - 10 cascaded shadow map (`sampler2DArrayShadow`, 4 layers, hardware PCF comparison)
  - 11 Light UBO (direction, colour, `cascadeViewProj[4]`, `cascadeSplits`, IBL params, shadow params, environment params with CSM debug-tint flag in `.w`)
  - 12 irradiance cubemap
  - 13 prefiltered environment cubemap
  - 14 BRDF integration LUT (2D)
- Separate descriptor layouts for the skybox (SkyboxUBO + samplerCube + LightUBO), shadow (ShadowUBO with `lightViewProj[4]` + SkinUBO + MorphUBO + MorphTargets SSBO, plus a `ShadowPushConstants { int cascadeIndex }` push-constant range on the vertex stage), and post-process (single combined image sampler on the HDR target) passes
- Three forward pipeline variants share the shader + binding layout but differ in cull mode, blend, and depth-write state:
  - **opaque** (cull back, no blend, depth write) — OPAQUE and MASK materials with `doubleSided=false`
  - **opaque-double-sided** (cull none, no blend, depth write) — OPAQUE and MASK with `doubleSided=true`
  - **blend** (cull none, `SRC_ALPHA / ONE_MINUS_SRC_ALPHA` blend, no depth write) — BLEND materials
- Additional persistent pipelines: **skybox** (fullscreen triangle, LEQUAL depth, no write), **shadow** (front-face cull, depth bias enabled, color write off), **post-process** (ACES + gamma 2.2, no depth)
- Transient IBL pipelines (`environment_convert`, `irradiance_convolution`, `prefilter_environment`, `brdf_integration`) exist only during the startup precompute
- MASK is implemented via a fragment-shader `discard` when `alpha < alphaCutoff`; OPAQUE/BLEND write `alphaCutoff = 0.0` so the discard is inert
- Resources class owns all GPU resources and exposes opaque handles (including a pipeline registry, IBL cubemaps, BRDF LUT, and the shadow map)
- Each Object creates its own descriptor pool and sets via Resources
- Depth buffering and swapchain recreation on window resize (HDR framebuffers and post-process input samplers are rebuilt too)

### Vertex Shader Pipeline

1. **Morph targets** (if enabled): accumulates weighted position/normal deltas from the SSBO
2. **Skinning** (if enabled): blends joint matrices using per-vertex joint indices and weights
3. **Transform**: applies either the blended skin matrix or the model matrix to produce world-space position
4. **TBN construction**: transforms the per-vertex normal and tangent by the normal matrix, orthogonalises T against N, builds B via `cross(N, T) * tangent.w`, and passes `mat3(T, B, N)` to the fragment shader for tangent-space normal mapping

### Fragment Shader

The forward fragment shader implements a PBR Cook-Torrance BRDF (GGX + Schlick Fresnel + Smith G) for the directional light, adds diffuse IBL from the irradiance cubemap and specular IBL via the prefiltered cubemap + BRDF LUT split-sum **with Fdez-Aguera multi-scatter compensation** for energy-conserving rough conductors, samples normal / metallic-roughness / occlusion / emissive textures if bound (gated by `textureFlags`/`extraFlags`), and modulates the direct-light contribution by a 3×3 PCF shadow term that picks one of 4 cascades per fragment based on view-space depth and softens cascade boundaries with a 10% blend band.

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

Run the tests (766 tests):

```bash
./build/test_fire_engine
```

Run the application:

```bash
cd build && ./fireEngineApp
```

The app accepts two optional positional arguments:

```bash
./fireEngineApp <scene.gltf> <skybox.hdr>
```

Both fall back to built-in defaults when omitted.

## Dependencies

Managed via vcpkg:

- `vulkan-headers` — Vulkan API headers
- `fastgltf` — glTF 2.0 parser
- `stb` — image loading (stb_image, incl. HDR)
- `gtest` — Google Test framework

Also requires:

- System GLFW 3.3+
- Vulkan SDK (for `glslc` shader compiler)

## Assets

All glTF models are from the Khronos glTF Sample Models [repository](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main): AlphaBlendModeTest, AnimatedCube, AnimatedMorphCube, BoomBoxWithAxes, BoxAnimated, BrainStem, CesiumMan, DamagedHelmet, Fox, InterpolationTest, MetalRoughSpheres, MorphPrimitivesTest, OrientationTest, RecursiveSkeletons, RiggedSimple, TextureCoordinateTest, TextureLinearInterpolationTest, TextureSettingsTest, VertexColorTest.

HDR equirectangular skyboxes (`skybox.hdr`, `nightbox.hdr`) drive the IBL precompute.
