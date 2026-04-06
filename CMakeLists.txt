cmake_minimum_required(VERSION 3.20)

project(fire_engine
    VERSION 0.1.0
    DESCRIPTION "Vulkan demo in C++"
    LANGUAGES CXX
)

# --- Dependencies -------------------------------------------------------

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GTest REQUIRED)
find_package(Stb REQUIRED)

# --- Library ------------------------------------------------------------

add_library(fireengine SHARED
    src/core/model_loader.cpp
    src/core/shader_loader.cpp
    src/fire_engine.cpp
    src/graphics/geometry.cpp
    src/graphics/image.cpp
    src/graphics/material.cpp
    src/graphics/texture.cpp
    src/graphics_driver.cpp
    src/input/input.cpp
    src/platform/window.cpp
    src/scene/camera.cpp
    src/scene/node.cpp
    src/scene/scene_graph.cpp
    src/scene/transform.cpp
)

target_include_directories(fireengine PUBLIC ${PROJECT_SOURCE_DIR}/include ${Stb_INCLUDE_DIR})
target_compile_features(fireengine PUBLIC cxx_std_23)
target_compile_options(fireengine PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(fireengine PUBLIC Vulkan::Vulkan glfw)

# --- Executable ---------------------------------------------------------

add_executable(fireEngineApp
    src/platform/application.cpp
)

target_link_libraries(fireEngineApp PRIVATE fireengine Vulkan::Vulkan glfw)
target_compile_options(fireEngineApp PRIVATE -Wall -Wextra -Wpedantic)

# --- Shaders ------------------------------------------------------------

set(SHADER_SOURCES
    shaders/shader.vert
    shaders/shader.frag
)

foreach(shader IN LISTS SHADER_SOURCES)
  get_filename_component(SHADER_NAME ${shader} NAME)
  set(SHADER_SPV ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv)

  add_custom_command(
    OUTPUT ${SHADER_SPV}
    COMMAND glslc ${PROJECT_SOURCE_DIR}/${shader} -o ${SHADER_SPV}
    DEPENDS ${PROJECT_SOURCE_DIR}/${shader}
    COMMENT "Compiling ${SHADER_NAME} -> ${SHADER_NAME}.spv"
    VERBATIM
  )

  list(APPEND SHADER_SPVS ${SHADER_SPV})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_SPVS} COMMENT "Building shaders")

# --- Assets -------------------------------------------------------------

add_custom_target(assets ALL
  COMMAND ${CMAKE_COMMAND}
    -DASSET_DIR=${PROJECT_SOURCE_DIR}/assets
    -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P ${PROJECT_SOURCE_DIR}/cmake/copy_assets.cmake
  COMMENT "Copying assets to build directory"
  VERBATIM
)

# --- Tests --------------------------------------------------------------

enable_testing()

add_executable(test_fire_engine
    tests/math/test_mat4.cpp
    tests/math/test_vec3.cpp
    tests/graphics/test_colour3.cpp
    tests/graphics/test_geometry.cpp
    tests/graphics/test_image.cpp
    tests/graphics/test_material.cpp
    tests/graphics/test_model_loader.cpp
    tests/core/test_shader_loader.cpp
    tests/scene/test_camera.cpp
)
target_link_libraries(test_fire_engine PRIVATE fireengine GTest::gtest_main)
target_compile_options(test_fire_engine PRIVATE -Wall -Wextra -Wpedantic)

file(GLOB_RECURSE TEST_ASSETS "${PROJECT_SOURCE_DIR}/tests/assets/*")
foreach(f IN LISTS TEST_ASSETS)
  get_filename_component(name "${f}" NAME)
  configure_file("${f}" "${CMAKE_CURRENT_BINARY_DIR}/test_assets/${name}" COPYONLY)
endforeach()

include(GoogleTest)
gtest_discover_tests(test_fire_engine)
