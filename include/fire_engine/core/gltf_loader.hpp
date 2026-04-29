#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <fastgltf/core.hpp>

#include <fire_engine/animation/animation.hpp>
#include <fire_engine/core/tangent_generator.hpp>
#include <fire_engine/graphics/image.hpp>
#include <fire_engine/math/mat4.hpp>
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
class Mesh;

class GltfLoader
{
public:
    GltfLoader() = delete;

    // glTF camera adoption. CameraView is the resolved viewpoint extracted
    // from a node carrying a `camera` index — position in world space and a
    // target point one unit ahead along the camera's forward (-Z in local).
    struct CameraView
    {
        Vec3 position;
        Vec3 target;
    };

    // Returns the first authored camera's view (if any) so callers can frame
    // the engine's runtime camera to match. Nullopt for assets without an
    // attached camera node.
    static std::optional<CameraView>
    loadScene(const std::string& path, SceneGraph& scene, Resources& resources, Assets& assets);

    // Synthesises per-vertex normals from a triangle mesh when the source
    // glTF lacks the NORMAL attribute. Smooth (area-weighted accumulate-and-
    // normalize) so curved meshes look right; the spec's "flat normals"
    // wording would require de-indexing and produces visibly worse results
    // on real assets like Fox.gltf.
    [[nodiscard]] static std::vector<Vec3> generateSmoothNormals(std::span<const Vec3> positions,
                                                                 std::span<const uint32_t> indices);

    // glTF 2.0 §3.2: implementations MUST refuse to load assets whose
    // `extensionsRequired` lists anything they don't support. Throws a
    // std::runtime_error naming each unsupported extension. Anything not
    // declared `required` (only `used`) is informational and ignored here.
    // Span of string_view so the call site can pass either fastgltf's
    // pmr-allocated strings or a plain std::vector from a unit test.
    static void ensureSupportedExtensions(std::span<const std::string_view> required);

    // Triangles only. Other primitive modes (POINTS / LINES / *_STRIP /
    // *_FAN) would need different vertex layout and index handling — we skip
    // the primitive with a warning rather than render garbage.
    [[nodiscard]] static bool isSupportedPrimitiveType(fastgltf::PrimitiveType type) noexcept;

    // Pure: derive the view from a node's accumulated world transform.
    // glTF cameras look down -Z in local space, so position is the
    // translation column and forward is the negated upper-3x3 third column,
    // re-normalised to wash out any node scale.
    [[nodiscard]] static CameraView cameraViewFromMatrix(const Mat4& world) noexcept;

    // DFS over the asset's default scene; returns the first node bearing a
    // camera index, or nullopt if none. Accumulates world transforms so the
    // returned view matches the artist's intent regardless of nesting.
    [[nodiscard]] static std::optional<CameraView>
    findFirstCamera(const fastgltf::Asset& asset);

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
    static const Texture*
    resolveTransmissionTexture(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                               const std::string& baseDir, Resources& resources, Assets& assets);

    [[nodiscard]]
    static Material* resolveMaterial(Material materialData, Assets& assets);

    [[nodiscard]]
    static TangentGenerationResult
    loadGeometry(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                 bool needsTangents, Resources& resources, Assets& assets, std::size_t geoIdx);

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
