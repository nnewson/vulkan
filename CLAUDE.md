# CLAUDE.md

## Project Overview

**fire_engine** is a Vulkan-based 3D renderer written in C++23. It loads OBJ/MTL models with texture mapping and renders them with Blinn-Phong lighting. Built on macOS with MoltenVK.

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

Managed via vcpkg: `vulkan-headers`, `gtest`, `stb`. Also requires system GLFW 3.3+ and the Vulkan SDK (for `glslc`).

## Project Structure

```
include/fire_engine/
  math/            # Vec3, Mat4, constants (header-only)
  graphics/        # Colour3, Vertex, Geometry, Material, Image, Texture, ModelLoader
  scene/           # Camera (header-only)
  graphics_driver.hpp
  fire_engine.hpp
  display.hpp
src/
  graphics/        # Implementations for graphics classes
  graphics_driver.cpp
  fire_engine.cpp
  display.cpp
shaders/           # GLSL vertex/fragment shaders
tests/
  math/            # test_vec3.cpp, test_mat4.cpp
  graphics/        # test_colour3.cpp, test_image.cpp, test_geometry.cpp, test_material.cpp, test_model_loader.cpp
  scene/           # test_camera.cpp
  assets/          # Minimal OBJ, MTL, PNG files for testing
assets/            # OBJ, MTL, PNG, JPG model assets
cmake/             # Build-time scripts (copy_assets.cmake)
```

## Code Style

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
- Shared file-parsing utilities in `ModelLoader` (trim, trim_comment, parse_file)
- Header-only math and scene types (Vec3, Mat4, Camera); other classes split into header + source
- Explicit rule-of-five on all classes (defaulted or deleted as appropriate)

## Testing

Google Test framework. All tests in a single executable `test_fire_engine`. Test files mirror the source structure: `tests/math/`, `tests/graphics/`, `tests/scene/`. Test assets (minimal OBJ, MTL, PNG files) live in `tests/assets/` and are copied to `build/test_assets/` at configure time. Tests cover construction, accessors, equality, arithmetic, file loading, move/copy semantics, edge cases, constexpr validation, and noexcept guarantees.

## Rendering Pipeline

- Vulkan with vulkan.hpp (C++ bindings)
- Descriptor set layout: binding 0 (UBO: model/view/proj/cameraPos), binding 1 (MaterialUBO), binding 2 (texture sampler)
- OBJ loading with vt/vn support, fan triangulation, automatic normal computation
- Texture loading via stb_image (RGBA), uploaded to GPU via staging buffer
- GLFW for windowing with keyboard (WASD/QE) and mouse camera controls
