#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <fastgltf/core.hpp>

#include <fire_engine/animation/linear_animation.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Animator;
class Assets;
class Geometry;
class LinearAnimation;
class Material;
class Node;
class Object;
class Renderer;
class SceneGraph;
class Texture;

class GltfLoader
{
public:
    GltfLoader() = delete;

    static void loadScene(const std::string& path, SceneGraph& scene, const Renderer& renderer,
                          Assets& assets);

private:
    // Asset parsing and setup
    [[nodiscard]]
    static fastgltf::Expected<fastgltf::Asset> parseAsset(const std::filesystem::path& gltfPath);

    static void presizeAssets(const fastgltf::Asset& asset, Assets& assets);

    // Node helpers
    [[nodiscard]]
    static Vec3 quaternionToEuler(float qx, float qy, float qz, float qw);

    static void applyTRS(const fastgltf::Node& gltfNode, Node& node);

    [[nodiscard]]
    static std::string descendantMeshName(const fastgltf::Asset& asset,
                                          const fastgltf::Node& gltfNode);

    [[nodiscard]]
    static std::string nodeName(const fastgltf::Asset& asset, const fastgltf::Node& gltfNode);

    static void configureAnimatedNode(const fastgltf::Asset& asset, std::size_t nodeIndex,
                                      Node& node, const std::string& baseDir,
                                      const Renderer& renderer, Assets& assets);

    static void loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& parentNode,
                         const std::string& baseDir, const Renderer& renderer, Assets& assets);

    // Mesh loading
    [[nodiscard]]
    static Object loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                           const std::string& baseDir, const Renderer& renderer, Assets& assets,
                           std::size_t meshIndex);

    [[nodiscard]]
    static const Texture*
    resolveTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                   const std::string& texturePath, const Renderer& renderer, Assets& assets);

    [[nodiscard]]
    static Material* resolveMaterial(const fastgltf::Asset& asset,
                                     const fastgltf::Primitive& primitive, Material& materialData,
                                     const Texture* texPtr, Assets& assets);

    static void loadGeometry(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                             const Material* matPtr, const Renderer& renderer, Assets& assets,
                             std::size_t geoIdx);

    [[nodiscard]]
    static std::pair<Material, std::string> loadMaterial(const fastgltf::Asset& asset,
                                                         const fastgltf::Primitive& primitive,
                                                         const std::string& baseDir);

    // Animation
    [[nodiscard]]
    static bool nodeHasAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex);

    static void loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                              Animator& animator);

    [[nodiscard]]
    static float computeSharedDuration(const fastgltf::Asset& asset, std::size_t nodeIndex);

    [[nodiscard]]
    static std::vector<LinearAnimation::RotationKeyframe>
    loadRotationKeyframes(const fastgltf::Asset& asset, const fastgltf::AnimationSampler& sampler);

    [[nodiscard]]
    static std::vector<LinearAnimation::TranslationKeyframe>
    loadTranslationKeyframes(const fastgltf::Asset& asset,
                             const fastgltf::AnimationSampler& sampler);
};

} // namespace fire_engine
