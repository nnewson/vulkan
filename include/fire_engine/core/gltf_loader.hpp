#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include <fastgltf/core.hpp>

namespace fire_engine
{

class Animator;
class Material;
class Node;
class Renderer;
class SceneGraph;

class GltfLoader
{
public:
    GltfLoader() = delete;

    static void loadScene(const std::string& path, SceneGraph& scene, const Renderer& renderer);

private:
    static void loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& parentNode,
                         const std::string& baseDir, const Renderer& renderer);

    static void loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, Node& node,
                         const std::string& baseDir, const Renderer& renderer);

    [[nodiscard]]
    static bool nodeHasAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex);

    static void loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                              Animator& animator);

    [[nodiscard]]
    static std::pair<Material, std::string>
    loadMaterial(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                 const std::string& baseDir);
};

} // namespace fire_engine
