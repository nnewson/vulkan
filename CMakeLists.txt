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

# --- Library ------------------------------------------------------------

add_library(fireengine SHARED
    src/display.cpp
    src/fire_engine.cpp
    src/geometry.cpp
    src/graphics_driver.cpp
    src/material.cpp
    src/math.cpp
)

target_include_directories(fireengine PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(fireengine PUBLIC cxx_std_23)
target_compile_options(fireengine PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(fireengine PUBLIC Vulkan::Vulkan glfw)

# --- Executable ---------------------------------------------------------

add_executable(fireEngineApp
    src/main.cpp
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

add_custom_command(
  TARGET fireEngineApp POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    -DASSET_DIR=${PROJECT_SOURCE_DIR}/assets
    -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P ${PROJECT_SOURCE_DIR}/cmake/copy_assets.cmake
  COMMENT "Copying assets to build directory"
  VERBATIM
)

# --- Tests --------------------------------------------------------------

enable_testing()

add_executable(test_fire_engine tests/test_fire_engine.cpp)
target_link_libraries(test_fire_engine PRIVATE fireengine GTest::gtest_main)
target_compile_options(test_fire_engine PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(test_fire_engine)
