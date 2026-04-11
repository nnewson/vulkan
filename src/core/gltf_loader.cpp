#include "fire_engine/render/renderer.hpp"
#include <fire_engine/core/gltf_loader.hpp>

#include <cmath>
#include <filesystem>
#include <stdexcept>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <fire_engine/animation/linear_animation.hpp>
#include <fire_engine/graphics/assets.hpp>
#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/object.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/scene/animator.hpp>
#include <fire_engine/scene/mesh.hpp>
#include <fire_engine/scene/node.hpp>
#include <fire_engine/scene/scene_graph.hpp>

namespace fire_engine
{

// ---------------------------------------------------------------------------
// Node helpers (formerly anonymous namespace)
// ---------------------------------------------------------------------------

Vec3 GltfLoader::quaternionToEuler(float qx, float qy, float qz, float qw)
{
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (qw * qy - qz * qx);
    float pitch =
        (std::abs(sinp) >= 1.0f) ? std::copysign(3.14159265f / 2.0f, sinp) : std::asin(sinp);

    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return {pitch, yaw, roll};
}

void GltfLoader::applyTRS(const fastgltf::Node& gltfNode, Node& node)
{
    if (auto* trs = std::get_if<fastgltf::TRS>(&gltfNode.transform))
    {
        node.transform().position(
            {trs->translation.x(), trs->translation.y(), trs->translation.z()});
        node.transform().rotation(quaternionToEuler(trs->rotation.x(), trs->rotation.y(),
                                                    trs->rotation.z(), trs->rotation.w()));
        node.transform().scale({trs->scale.x(), trs->scale.y(), trs->scale.z()});
    }
}

std::string GltfLoader::descendantMeshName(const fastgltf::Asset& asset,
                                           const fastgltf::Node& gltfNode)
{
    if (gltfNode.meshIndex.has_value())
    {
        const auto& name = asset.meshes[gltfNode.meshIndex.value()].name;
        if (!name.empty())
            return std::string(name);
    }
    for (auto childIndex : gltfNode.children)
    {
        auto result = descendantMeshName(asset, asset.nodes[childIndex]);
        if (!result.empty())
            return result;
    }
    return {};
}

std::string GltfLoader::nodeName(const fastgltf::Asset& asset, const fastgltf::Node& gltfNode)
{
    if (!gltfNode.name.empty())
        return std::string(gltfNode.name);
    auto meshName = descendantMeshName(asset, gltfNode);
    if (!meshName.empty())
        return meshName;
    return "Node";
}

// ---------------------------------------------------------------------------
// Asset parsing and setup
// ---------------------------------------------------------------------------

fastgltf::Expected<fastgltf::Asset> GltfLoader::parseAsset(const std::filesystem::path& gltfPath)
{
    fastgltf::Parser parser;
    auto dataResult = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    if (dataResult.error() != fastgltf::Error::None)
    {
        throw std::runtime_error("Failed to read glTF file: " +
                                 std::string(fastgltf::getErrorMessage(dataResult.error())));
    }

    auto options = fastgltf::Options::LoadExternalBuffers;
    auto result = parser.loadGltf(dataResult.get(), gltfPath.parent_path(), options);
    if (result.error() != fastgltf::Error::None)
    {
        throw std::runtime_error("Failed to load glTF: " +
                                 std::string(fastgltf::getErrorMessage(result.error())));
    }

    return result;
}

void GltfLoader::presizeAssets(const fastgltf::Asset& asset, Assets& assets)
{
    assets.resizeTextures(std::max<std::size_t>(asset.textures.size(), 1));
    assets.resizeMaterials(std::max<std::size_t>(asset.materials.size(), 1));

    std::size_t totalPrimitives = 0;
    for (const auto& m : asset.meshes)
        totalPrimitives += m.primitives.size();
    assets.resizeGeometries(totalPrimitives);
}

// ---------------------------------------------------------------------------
// Scene and node loading
// ---------------------------------------------------------------------------

void GltfLoader::loadScene(const std::string& path, SceneGraph& scene, const Renderer& renderer,
                           Assets& assets)
{
    auto gltfPath = std::filesystem::path(path);
    auto result = parseAsset(gltfPath);
    auto& asset = result.get();

    presizeAssets(asset, assets);

    std::size_t sceneIndex = asset.defaultScene.has_value() ? asset.defaultScene.value() : 0;
    if (sceneIndex >= asset.scenes.size())
    {
        throw std::runtime_error("glTF scene index out of range");
    }

    const auto& gltfScene = asset.scenes[sceneIndex];
    for (auto nodeIndex : gltfScene.nodeIndices)
    {
        auto rootNode = std::make_unique<Node>(nodeName(asset, asset.nodes[nodeIndex]));
        auto& rootRef = scene.addNode(std::move(rootNode));

        if (nodeHasAnimation(asset, nodeIndex))
        {
            configureAnimatedNode(asset, nodeIndex, rootRef, gltfPath.parent_path().string(),
                                  renderer, assets);
        }
        else
        {
            loadNode(asset, nodeIndex, rootRef, gltfPath.parent_path().string(), renderer, assets);
        }
    }
}

void GltfLoader::configureAnimatedNode(const fastgltf::Asset& asset, std::size_t nodeIndex,
                                       Node& node, const std::string& baseDir,
                                       const Renderer& renderer, Assets& assets)
{
    const auto& gltfNode = asset.nodes[nodeIndex];
    applyTRS(gltfNode, node);

    node.component().emplace<Animator>();
    loadAnimation(asset, nodeIndex, std::get<Animator>(node.component()));

    if (gltfNode.meshIndex.has_value())
    {
        const auto& gltfMesh = asset.meshes[gltfNode.meshIndex.value()];
        std::string meshName = gltfMesh.name.empty() ? std::string(gltfNode.name) + "_Mesh"
                                                     : std::string(gltfMesh.name);
        auto meshNode = std::make_unique<Node>(std::move(meshName));
        auto& meshRef = node.addChild(std::move(meshNode));
        auto object = loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()], baseDir, renderer,
                               assets, gltfNode.meshIndex.value());
        meshRef.component().emplace<Mesh>(std::move(object));
    }

    for (auto childIndex : gltfNode.children)
    {
        loadNode(asset, childIndex, node, baseDir, renderer, assets);
    }
}

void GltfLoader::loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& node,
                          const std::string& baseDir, const Renderer& renderer, Assets& assets)
{
    const auto& gltfNode = asset.nodes[nodeIndex];
    applyTRS(gltfNode, node);

    if (gltfNode.meshIndex.has_value())
    {
        auto object = loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()], baseDir, renderer,
                               assets, gltfNode.meshIndex.value());
        node.component().emplace<Mesh>(std::move(object));
    }

    for (auto childIndex : gltfNode.children)
    {
        auto childNode = std::make_unique<Node>(nodeName(asset, asset.nodes[childIndex]));
        auto& childRef = node.addChild(std::move(childNode));

        if (nodeHasAnimation(asset, childIndex))
        {
            // TODO: glTF spec says animated channels *replace* the corresponding static
            // TRS component. We currently apply the full static TRS here and then let
            // the Animator compose its sampled matrix on top, which double-applies if an
            // asset has both a non-identity static translation/rotation and an animation
            // on the same component. Fix by skipping animated channels when applying.
            configureAnimatedNode(asset, childIndex, childRef, baseDir, renderer, assets);
        }
        else
        {
            loadNode(asset, childIndex, childRef, baseDir, renderer, assets);
        }
    }
}

// ---------------------------------------------------------------------------
// Mesh loading helpers
// ---------------------------------------------------------------------------

const Texture* GltfLoader::resolveTexture(const fastgltf::Asset& asset,
                                          const fastgltf::Primitive& primitive,
                                          const std::string& texturePath, const Renderer& renderer,
                                          Assets& assets)
{
    if (primitive.materialIndex.has_value())
    {
        const auto& gltfMat = asset.materials[primitive.materialIndex.value()];
        if (gltfMat.pbrData.baseColorTexture.has_value())
        {
            auto texIndex = gltfMat.pbrData.baseColorTexture.value().textureIndex;
            auto& tex = assets.texture(texIndex);
            if (tex.view() == vk::ImageView{})
            {
                std::string texPath = texturePath.empty() ? "default.png" : texturePath;
                auto& device = renderer.device();
                tex =
                    Texture::load_from_file(texPath, device.device(), device.physicalDevice(),
                                            renderer.frame().commandPool(), device.graphicsQueue());
            }
            return &tex;
        }
    }

    auto& defaultTex = assets.texture(0);
    if (defaultTex.view() == vk::ImageView{})
    {
        auto& device = renderer.device();
        defaultTex =
            Texture::load_from_file("default.png", device.device(), device.physicalDevice(),
                                    renderer.frame().commandPool(), device.graphicsQueue());
    }
    return &defaultTex;
}

Material* GltfLoader::resolveMaterial(const fastgltf::Asset& /* asset */,
                                      const fastgltf::Primitive& primitive, Material& materialData,
                                      const Texture* texPtr, Assets& assets)
{
    if (primitive.materialIndex.has_value())
    {
        auto matIndex = primitive.materialIndex.value();
        auto& mat = assets.material(matIndex);
        if (!mat.hasTexture())
        {
            mat = materialData;
            mat.texture(texPtr);
        }
        return &mat;
    }

    auto& mat = assets.material(0);
    if (!mat.hasTexture())
    {
        mat.texture(texPtr);
    }
    return &mat;
}

void GltfLoader::loadGeometry(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                              const Material* matPtr, const Renderer& renderer, Assets& assets,
                              std::size_t geoIdx)
{
    auto& geometry = assets.geometry(geoIdx);
    if (geometry.loaded())
        return;

    const auto* posAttr = primitive.findAttribute("POSITION");
    const auto* normAttr = primitive.findAttribute("NORMAL");
    const auto* uvAttr = primitive.findAttribute("TEXCOORD_0");

    if (posAttr == primitive.attributes.end())
    {
        throw std::runtime_error("glTF primitive missing POSITION attribute");
    }

    const auto& posAccessor = asset.accessors[posAttr->accessorIndex];
    std::vector<fastgltf::math::fvec3> positions(posAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, posAccessor,
        [&](fastgltf::math::fvec3 pos, std::size_t idx) { positions[idx] = pos; });

    std::vector<fastgltf::math::fvec3> normals;
    if (normAttr != primitive.attributes.end())
    {
        const auto& normAccessor = asset.accessors[normAttr->accessorIndex];
        normals.resize(normAccessor.count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset, normAccessor,
            [&](fastgltf::math::fvec3 n, std::size_t idx) { normals[idx] = n; });
    }

    std::vector<fastgltf::math::fvec2> texcoords;
    if (uvAttr != primitive.attributes.end())
    {
        const auto& uvAccessor = asset.accessors[uvAttr->accessorIndex];
        texcoords.resize(uvAccessor.count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
            asset, uvAccessor,
            [&](fastgltf::math::fvec2 uv, std::size_t idx) { texcoords[idx] = uv; });
    }

    Colour3 vertexColour = matPtr->diffuse();

    std::vector<Vertex> verts;
    verts.reserve(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        Vec3 pos{positions[i].x(), positions[i].y(), positions[i].z()};
        Vec3 norm = (i < normals.size()) ? Vec3{normals[i].x(), normals[i].y(), normals[i].z()}
                                         : Vec3{0.0f, 1.0f, 0.0f};
        float u = (i < texcoords.size()) ? texcoords[i].x() : 0.0f;
        float v = (i < texcoords.size()) ? texcoords[i].y() : 0.0f;

        verts.push_back(Vertex{pos, vertexColour, norm, u, v});
    }
    geometry.vertices(std::move(verts));

    std::vector<uint16_t> idxs;
    if (primitive.indicesAccessor.has_value())
    {
        const auto& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
        idxs.reserve(indexAccessor.count);
        fastgltf::iterateAccessor<std::uint32_t>(asset, indexAccessor, [&](std::uint32_t idx)
                                                 { idxs.push_back(static_cast<uint16_t>(idx)); });
    }
    else
    {
        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            idxs.push_back(static_cast<uint16_t>(i));
        }
    }
    geometry.indices(std::move(idxs));
    geometry.material(matPtr);
    geometry.load(renderer);
}

Object GltfLoader::loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                            const std::string& baseDir, const Renderer& renderer, Assets& assets,
                            std::size_t meshIndex)
{
    std::size_t geoStartIdx = 0;
    for (std::size_t m = 0; m < meshIndex; ++m)
        geoStartIdx += asset.meshes[m].primitives.size();

    Object object;

    for (std::size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx)
    {
        const auto& primitive = mesh.primitives[primIdx];
        auto [materialData, texturePath] = loadMaterial(asset, primitive, baseDir);

        const Texture* texPtr = resolveTexture(asset, primitive, texturePath, renderer, assets);
        Material* matPtr = resolveMaterial(asset, primitive, materialData, texPtr, assets);

        std::size_t geoIdx = geoStartIdx + primIdx;
        loadGeometry(asset, primitive, matPtr, renderer, assets, geoIdx);
        object.addGeometry(assets.geometry(geoIdx));
    }

    object.load(renderer);
    return object;
}

std::pair<Material, std::string> GltfLoader::loadMaterial(const fastgltf::Asset& asset,
                                                          const fastgltf::Primitive& primitive,
                                                          const std::string& baseDir)
{
    Material material;
    std::string texturePath;

    if (primitive.materialIndex.has_value())
    {
        const auto& gltfMat = asset.materials[primitive.materialIndex.value()];
        material.name(std::string(gltfMat.name));

        const auto& pbr = gltfMat.pbrData;
        material.diffuse({static_cast<float>(pbr.baseColorFactor.x()),
                          static_cast<float>(pbr.baseColorFactor.y()),
                          static_cast<float>(pbr.baseColorFactor.z())});
        material.metallic(static_cast<float>(pbr.metallicFactor));
        material.roughness(static_cast<float>(pbr.roughnessFactor));

        material.emissive({static_cast<float>(gltfMat.emissiveFactor.x()),
                           static_cast<float>(gltfMat.emissiveFactor.y()),
                           static_cast<float>(gltfMat.emissiveFactor.z())});

        // Resolve base color texture path
        if (pbr.baseColorTexture.has_value())
        {
            auto texIndex = pbr.baseColorTexture.value().textureIndex;
            const auto& tex = asset.textures[texIndex];
            if (tex.imageIndex.has_value())
            {
                const auto& img = asset.images[tex.imageIndex.value()];
                if (auto* uri = std::get_if<fastgltf::sources::URI>(&img.data))
                {
                    texturePath = baseDir + "/" + std::string(uri->uri.path());
                }
            }
        }
    }

    return {std::move(material), std::move(texturePath)};
}

// ---------------------------------------------------------------------------
// Animation helpers
// ---------------------------------------------------------------------------

bool GltfLoader::nodeHasAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex)
{
    for (const auto& anim : asset.animations)
    {
        for (const auto& channel : anim.channels)
        {
            if (channel.nodeIndex.has_value() && channel.nodeIndex.value() == nodeIndex)
            {
                return true;
            }
        }
    }
    return false;
}

float GltfLoader::computeSharedDuration(const fastgltf::Asset& asset, std::size_t nodeIndex)
{
    float sharedDuration = 0.0f;

    for (const auto& anim : asset.animations)
    {
        bool touchesNode = false;
        for (const auto& channel : anim.channels)
        {
            if (channel.nodeIndex.has_value() && channel.nodeIndex.value() == nodeIndex)
            {
                touchesNode = true;
                break;
            }
        }
        if (!touchesNode)
            continue;

        for (const auto& channel : anim.channels)
        {
            const auto& sampler = anim.samplers[channel.samplerIndex];
            const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
            fastgltf::iterateAccessor<float>(asset, inputAccessor, [&](float t)
                                             { sharedDuration = std::max(sharedDuration, t); });
        }
    }

    return sharedDuration;
}

std::vector<LinearAnimation::RotationKeyframe>
GltfLoader::loadRotationKeyframes(const fastgltf::Asset& asset,
                                  const fastgltf::AnimationSampler& sampler)
{
    const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

    std::vector<float> times(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor,
                                              [&](float t, std::size_t idx) { times[idx] = t; });

    std::vector<fastgltf::math::fvec4> quats(outputAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
        asset, outputAccessor, [&](fastgltf::math::fvec4 q, std::size_t idx) { quats[idx] = q; });

    std::vector<LinearAnimation::RotationKeyframe> kf;
    std::size_t count = std::min(times.size(), quats.size());
    kf.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        kf.push_back({times[i], quats[i].x(), quats[i].y(), quats[i].z(), quats[i].w()});
    }
    return kf;
}

std::vector<LinearAnimation::TranslationKeyframe>
GltfLoader::loadTranslationKeyframes(const fastgltf::Asset& asset,
                                     const fastgltf::AnimationSampler& sampler)
{
    const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

    std::vector<float> times(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor,
                                              [&](float t, std::size_t idx) { times[idx] = t; });

    std::vector<fastgltf::math::fvec3> positions(outputAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, outputAccessor,
        [&](fastgltf::math::fvec3 p, std::size_t idx) { positions[idx] = p; });

    std::vector<LinearAnimation::TranslationKeyframe> kf;
    std::size_t count = std::min(times.size(), positions.size());
    kf.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        kf.push_back({times[i], Vec3{positions[i].x(), positions[i].y(), positions[i].z()}});
    }
    return kf;
}

void GltfLoader::loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                               Animator& animator)
{
    auto& la = animator.animation();
    float sharedDuration = computeSharedDuration(asset, nodeIndex);

    for (const auto& anim : asset.animations)
    {
        for (const auto& channel : anim.channels)
        {
            if (!channel.nodeIndex.has_value() || channel.nodeIndex.value() != nodeIndex)
                continue;

            const auto& sampler = anim.samplers[channel.samplerIndex];

            if (channel.path == fastgltf::AnimationPath::Rotation)
            {
                la.rotationKeyframes(loadRotationKeyframes(asset, sampler));
            }
            else if (channel.path == fastgltf::AnimationPath::Translation)
            {
                la.translationKeyframes(loadTranslationKeyframes(asset, sampler));
            }
        }
    }

    la.duration(sharedDuration);
}

} // namespace fire_engine
