#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>

#include <fastgltf/core.hpp>

#include <fire_engine/core/tangent_generator.hpp>
#include <fire_engine/animation/animation.hpp>
#include <fire_engine/graphics/image.hpp>

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
class Mesh;

class GltfLoader
{
public:
    GltfLoader() = delete;

    static void loadScene(const std::string& path, SceneGraph& scene, Resources& resources,
                          Assets& assets);

    struct TexturePaths
    {
        std::string baseColour;
        std::string emissive;
        std::string normal;
        std::string metallicRoughness;
        std::string occlusion;
    };

    using NodeMap = std::unordered_map<std::size_t, Node*>;
    using MeshMap = std::unordered_map<std::size_t, Mesh*>;

private:
    // Asset parsing and setup
    [[nodiscard]]
    static fastgltf::Expected<fastgltf::Asset> parseAsset(const std::filesystem::path& gltfPath);

    static void presizeAssets(const fastgltf::Asset& asset, Assets& assets);

    // Node helpers
    static void applyTRS(const fastgltf::Node& gltfNode, Node& node);

    [[nodiscard]]
    static std::string descendantMeshName(const fastgltf::Asset& asset,
                                          const fastgltf::Node& gltfNode);

    [[nodiscard]]
    static std::string nodeName(const fastgltf::Asset& asset, const fastgltf::Node& gltfNode);

    static void configureAnimatedNode(const fastgltf::Asset& asset, std::size_t nodeIndex,
                                      Node& node, const std::string& baseDir, Resources& resources,
                                      Assets& assets, NodeMap& nodeMap, MeshMap& meshMap,
                                      std::size_t& nextAnimSlot);

    static void loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& parentNode,
                         const std::string& baseDir, Resources& resources, Assets& assets,
                         NodeMap& nodeMap, MeshMap& meshMap, std::size_t& nextAnimSlot);

    // Skin loading
    static void loadSkin(const fastgltf::Asset& asset, std::size_t skinIndex,
                         const NodeMap& nodeMap, Assets& assets);

    static void applySkins(const fastgltf::Asset& asset, const NodeMap& nodeMap,
                           const MeshMap& meshMap, Assets& assets);

    // Mesh loading
    [[nodiscard]]
    static Object loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                           const std::string& baseDir, Resources& resources, Assets& assets,
                           std::size_t meshIndex);

    [[nodiscard]]
    static const Texture*
    resolveTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                   const std::string& baseDir, Resources& resources, Assets& assets);

    [[nodiscard]]
    static const Texture*
    resolveEmissiveTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                           const std::string& baseDir, Resources& resources, Assets& assets);

    [[nodiscard]]
    static const Texture*
    resolveNormalTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                         const std::string& baseDir, Resources& resources, Assets& assets);

    [[nodiscard]]
    static const Texture* resolveMetallicRoughnessTexture(const fastgltf::Asset& asset,
                                                          const fastgltf::Primitive& primitive,
                                                          const std::string& baseDir,
                                                          Resources& resources, Assets& assets);

    [[nodiscard]]
    static const Texture*
    resolveOcclusionTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                            const std::string& baseDir, Resources& resources, Assets& assets);

    [[nodiscard]]
    static Material* resolveMaterial(Material materialData, Assets& assets);

    [[nodiscard]]
    static TangentGenerationResult loadGeometry(const fastgltf::Asset& asset,
                                                const fastgltf::Primitive& primitive,
                                                bool needsTangents, Resources& resources,
                                                Assets& assets, std::size_t geoIdx);

    [[nodiscard]]
    static Material loadMaterial(const fastgltf::Asset& asset,
                                 const fastgltf::Primitive& primitive);

    [[nodiscard]]
    static Image loadImage(const fastgltf::Asset& asset, std::size_t imageIndex,
                           const std::string& baseDir);

    // Animation
    static void applyRestTRS(const fastgltf::Node& gltfNode, Animation& anim);

    [[nodiscard]]
    static bool nodeHasAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex);

    static void loadAnimation(const fastgltf::Asset& asset, std::size_t gltfAnimIndex,
                              std::size_t nodeIndex, Animation& anim,
                              std::size_t numMorphTargets = 0);

    [[nodiscard]]
    static float computeSharedDuration(const fastgltf::Asset& asset, std::size_t gltfAnimIndex);

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
