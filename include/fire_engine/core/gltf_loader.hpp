#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fastgltf/core.hpp>

#include <fire_engine/animation/animation.hpp>
#include <fire_engine/math/vec3.hpp>

namespace fire_engine
{

class Animator;
class Assets;
class Geometry;
class Animation;
class Material;
class Node;
class Object;
class Resources;
class SceneGraph;
class Skin;
class Texture;

class GltfLoader
{
public:
    GltfLoader() = delete;

    static void loadScene(const std::string& path, SceneGraph& scene,
                          Resources& resources, Assets& assets);

    using NodeMap = std::unordered_map<std::size_t, Node*>;
    using AnimationMap = std::unordered_map<std::size_t, std::size_t>;

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
                                      Resources& resources, Assets& assets,
                                      NodeMap& nodeMap, AnimationMap& animMap);

    static void loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& parentNode,
                         const std::string& baseDir, Resources& resources, Assets& assets,
                         NodeMap& nodeMap, AnimationMap& animMap);

    // Skin loading
    static void loadSkin(const fastgltf::Asset& asset, std::size_t skinIndex,
                         const NodeMap& nodeMap, Assets& assets);

    static void applySkins(const fastgltf::Asset& asset, const NodeMap& nodeMap, Assets& assets);

    // Mesh loading
    [[nodiscard]]
    static Object loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                           const std::string& baseDir, Resources& resources, Assets& assets,
                           std::size_t meshIndex);

    [[nodiscard]]
    static const Texture*
    resolveTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                   const std::string& texturePath, Resources& resources, Assets& assets);

    [[nodiscard]]
    static Material* resolveMaterial(const fastgltf::Asset& asset,
                                     const fastgltf::Primitive& primitive, Material& materialData,
                                     const Texture* texPtr, Assets& assets);

    static void loadGeometry(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                             const Material* matPtr, Resources& resources, Assets& assets,
                             std::size_t geoIdx);

    [[nodiscard]]
    static std::pair<Material, std::string> loadMaterial(const fastgltf::Asset& asset,
                                                         const fastgltf::Primitive& primitive,
                                                         const std::string& baseDir);

    // Animation
    static void applyRestTRS(const fastgltf::Node& gltfNode, Animation& anim);

    [[nodiscard]]
    static bool nodeHasAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex);

    static void loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                              Animation& anim, std::size_t numMorphTargets = 0);

    [[nodiscard]]
    static float computeSharedDuration(const fastgltf::Asset& asset, std::size_t nodeIndex);

    [[nodiscard]]
    static Animation::Interpolation mapInterpolation(fastgltf::AnimationInterpolation m);

    static void loadRotationChannel(const fastgltf::Asset& asset,
                                    const fastgltf::AnimationSampler& sampler, Animation& anim);

    static void loadTranslationChannel(const fastgltf::Asset& asset,
                                       const fastgltf::AnimationSampler& sampler, Animation& anim);

    static void loadScaleChannel(const fastgltf::Asset& asset,
                                 const fastgltf::AnimationSampler& sampler, Animation& anim);

    static void loadWeightChannel(const fastgltf::Asset& asset,
                                  const fastgltf::AnimationSampler& sampler, Animation& anim,
                                  std::size_t numTargets);
};

} // namespace fire_engine
