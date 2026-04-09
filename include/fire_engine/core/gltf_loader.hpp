#pragma once

#include <cstddef>
#include <string>

#include <fastgltf/core.hpp>

namespace fire_engine
{

class Device;
class Frame;
class Pipeline;
class Animator;
class Node;
class SceneGraph;

class GltfLoader
{
public:
    GltfLoader() = delete;

    static void loadScene(const std::string& path, SceneGraph& scene, const Device& device,
                          const Pipeline& pipeline, Frame& frame);

private:
    static void loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& parentNode,
                         const std::string& baseDir, const Device& device, const Pipeline& pipeline,
                         Frame& frame);

    static void loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, Node& node,
                         const std::string& baseDir, const Device& device, const Pipeline& pipeline,
                         Frame& frame);

    [[nodiscard]]
    static bool nodeHasAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex);

    static void loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                              Animator& animator);
};

} // namespace fire_engine
