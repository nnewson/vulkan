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

Managed via vcpkg: `vulkan-headers`, `gtest`, `stb`, `tinyobjloader`. Also requires system GLFW 3.3+ and the Vulkan SDK (for `glslc`).

## Project Structure

```
include/fire_engine/
  core/            # ShaderLoader
  math/            # Vec3, Mat4, constants (header-only)
  graphics/        # Colour3, Vertex, Geometry, Material, Image, Texture
  platform/        # Window, Keyboard, Mouse (Keyboard/Mouse header-only)
  scene/           # Camera (header-only)
  graphics_driver.hpp
  fire_engine.hpp
src/
  core/            # shader_loader.cpp
  graphics/        # Implementations for graphics classes
  platform/        # window.cpp, application.cpp (main entry point)
  graphics_driver.cpp
  fire_engine.cpp
shaders/           # GLSL vertex/fragment shaders
tests/
  core/            # test_shader_loader.cpp
  math/            # test_vec3.cpp, test_mat4.cpp
  graphics/        # test_colour3.cpp, test_image.cpp, test_geometry.cpp, test_material.cpp
  scene/           # test_camera.cpp
  assets/          # Minimal OBJ, MTL, PNG, BIN files for testing
assets/            # OBJ, MTL, PNG, JPG model assets
cmake/             # Build-time scripts (copy_assets.cmake)
```

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
- OBJ/MTL loading via tinyobjloader (`Geometry::load_from_file`, `Material::load_from_file`)
- Shader loading via `core/ShaderLoader::load_from_file(path)`
- Header-only math, scene, and platform input types (Vec3, Mat4, Camera, Keyboard, Mouse); other classes split into header + source
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

Google Test framework. All tests in a single executable `test_fire_engine`. Test files mirror the source structure: `tests/math/`, `tests/graphics/`, `tests/scene/`. Test assets (minimal OBJ, MTL, PNG files) live in `tests/assets/` and are copied to `build/test_assets/` at configure time. Tests cover construction, accessors, equality, arithmetic, file loading, move/copy semantics, edge cases, constexpr validation, and noexcept guarantees.

## Rendering Pipeline

- Vulkan with vulkan.hpp (C++ bindings)
- Descriptor set layout: binding 0 (UBO: model/view/proj/cameraPos), binding 1 (MaterialUBO), binding 2 (texture sampler)
- OBJ/MTL loading via tinyobjloader with automatic triangulation and normal computation
- Texture loading via stb_image (RGBA), uploaded to GPU via staging buffer
- GLFW for windowing with keyboard (WASD/QE) and mouse camera controls
