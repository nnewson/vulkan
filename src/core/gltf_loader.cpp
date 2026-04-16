#include <fire_engine/core/gltf_loader.hpp>

#include <fire_engine/render/resources.hpp>

#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <unordered_set>

#include <fastgltf/core.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <fire_engine/animation/animation.hpp>
#include <fire_engine/graphics/assets.hpp>
#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/object.hpp>
#include <fire_engine/graphics/skin.hpp>
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
    else if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&gltfNode.transform))
    {
        fastgltf::math::fvec3 scale;
        fastgltf::math::fquat rotation;
        fastgltf::math::fvec3 translation;
        fastgltf::math::decomposeTransformMatrix(*mat, scale, rotation, translation);

        node.transform().position({translation.x(), translation.y(), translation.z()});
        node.transform().rotation(
            quaternionToEuler(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
        node.transform().scale({scale.x(), scale.y(), scale.z()});
    }
}

void GltfLoader::applyRestTRS(const fastgltf::Node& gltfNode, Animation& anim)
{
    fastgltf::math::fvec3 t{0.0f, 0.0f, 0.0f};
    fastgltf::math::fquat r{0.0f, 0.0f, 0.0f, 1.0f};
    fastgltf::math::fvec3 s{1.0f, 1.0f, 1.0f};

    if (auto* trs = std::get_if<fastgltf::TRS>(&gltfNode.transform))
    {
        t = trs->translation;
        r = trs->rotation;
        s = trs->scale;
    }
    else if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&gltfNode.transform))
    {
        fastgltf::math::decomposeTransformMatrix(*mat, s, r, t);
    }

    if (anim.translationKeyframes().empty())
    {
        anim.translationKeyframes({{0.0f, {t.x(), t.y(), t.z()}}});
    }
    if (anim.rotationKeyframes().empty())
    {
        anim.rotationKeyframes({{0.0f, r.x(), r.y(), r.z(), r.w()}});
    }
    if (anim.scaleKeyframes().empty())
    {
        anim.scaleKeyframes({{0.0f, {s.x(), s.y(), s.z()}}});
    }
}

std::string GltfLoader::descendantMeshName(const fastgltf::Asset& asset,
                                           const fastgltf::Node& gltfNode)
{
    if (gltfNode.meshIndex.has_value())
    {
        const auto& name = asset.meshes[gltfNode.meshIndex.value()].name;
        if (!name.empty())
        {
            return std::string(name);
        }
    }
    for (auto childIndex : gltfNode.children)
    {
        auto result = descendantMeshName(asset, asset.nodes[childIndex]);
        if (!result.empty())
        {
            return result;
        }
    }
    return {};
}

std::string GltfLoader::nodeName(const fastgltf::Asset& asset, const fastgltf::Node& gltfNode)
{
    if (!gltfNode.name.empty())
    {
        return std::string(gltfNode.name);
    }
    auto meshName = descendantMeshName(asset, gltfNode);
    if (!meshName.empty())
    {
        return meshName;
    }
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
    {
        totalPrimitives += m.primitives.size();
    }
    assets.resizeGeometries(totalPrimitives);
    assets.resizeSkins(asset.skins.size());

    // Count unique animated nodes for Animation slots
    std::unordered_set<std::size_t> animatedNodes;
    for (const auto& anim : asset.animations)
    {
        for (const auto& channel : anim.channels)
        {
            if (channel.nodeIndex.has_value())
            {
                animatedNodes.insert(channel.nodeIndex.value());
            }
        }
    }
    assets.resizeAnimations(animatedNodes.size());
}

// ---------------------------------------------------------------------------
// Scene and node loading
// ---------------------------------------------------------------------------

void GltfLoader::loadScene(const std::string& path, SceneGraph& scene, Resources& resources,
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
    NodeMap nodeMap;
    AnimationMap animMap;
    for (auto nodeIndex : gltfScene.nodeIndices)
    {
        auto rootNode = std::make_unique<Node>(nodeName(asset, asset.nodes[nodeIndex]));
        auto& rootRef = scene.addNode(std::move(rootNode));
        nodeMap[nodeIndex] = &rootRef;

        if (nodeHasAnimation(asset, nodeIndex))
        {
            configureAnimatedNode(asset, nodeIndex, rootRef, gltfPath.parent_path().string(),
                                  resources, assets, nodeMap, animMap);
        }
        else
        {
            loadNode(asset, nodeIndex, rootRef, gltfPath.parent_path().string(), resources, assets,
                     nodeMap, animMap);
        }
    }

    // Resolve skins after the full scene graph is built
    applySkins(asset, nodeMap, assets);
}

void GltfLoader::configureAnimatedNode(const fastgltf::Asset& asset, std::size_t nodeIndex,
                                       Node& node, const std::string& baseDir,
                                       Resources& resources, Assets& assets, NodeMap& nodeMap, AnimationMap& animMap)
{
    const auto& gltfNode = asset.nodes[nodeIndex];

    // Determine morph target count from the mesh (if any)
    std::size_t numMorphTargets = 0;
    if (gltfNode.meshIndex.has_value())
    {
        const auto& gltfMesh = asset.meshes[gltfNode.meshIndex.value()];
        if (!gltfMesh.primitives.empty() && !gltfMesh.primitives[0].targets.empty())
        {
            numMorphTargets = gltfMesh.primitives[0].targets.size();
        }
    }

    // Check if this node has transform vs weight animation channels
    bool hasTransformAnim = false;
    bool hasWeightAnim = false;
    for (const auto& anim : asset.animations)
    {
        for (const auto& channel : anim.channels)
        {
            if (!channel.nodeIndex.has_value() || channel.nodeIndex.value() != nodeIndex)
            {
                continue;
            }
            if (channel.path == fastgltf::AnimationPath::Rotation ||
                channel.path == fastgltf::AnimationPath::Translation ||
                channel.path == fastgltf::AnimationPath::Scale)
            {
                hasTransformAnim = true;
            }
            else if (channel.path == fastgltf::AnimationPath::Weights)
            {
                hasWeightAnim = true;
            }
        }
    }

    // Only apply rest TRS when animation won't drive it. glTF animation channels
    // replace (not compose with) the node's base TRS, so applying rest on top of
    // the animator's sampled matrix double-transforms the node.
    if (!hasTransformAnim)
    {
        applyTRS(gltfNode, node);
    }

    auto animIndex = animMap.size();
    animMap[nodeIndex] = animIndex;
    auto& la = assets.animation(animIndex);
    loadAnimation(asset, nodeIndex, la, numMorphTargets);
    applyRestTRS(gltfNode, la);

    if (hasTransformAnim)
    {
        // Node gets an Animator for transform; mesh goes on a child node
        node.component().emplace<Animator>();
        std::get<Animator>(node.component()).animation(&la);

        if (gltfNode.meshIndex.has_value())
        {
            const auto& gltfMesh = asset.meshes[gltfNode.meshIndex.value()];
            std::string meshName = gltfMesh.name.empty() ? std::string(gltfNode.name) + "_Mesh"
                                                         : std::string(gltfMesh.name);
            auto meshNode = std::make_unique<Node>(std::move(meshName));
            auto& meshRef = node.addChild(std::move(meshNode));
            auto object = loadMesh(asset, gltfMesh, baseDir, resources, assets,
                                   gltfNode.meshIndex.value());
            meshRef.component().emplace<Mesh>(std::move(object));

            if (hasWeightAnim)
            {
                auto& mesh = std::get<Mesh>(meshRef.component());
                mesh.morphAnimation(&la);
                mesh.initialMorphWeights(std::vector<float>(numMorphTargets, 0.0f));
            }
        }
    }
    else if (gltfNode.meshIndex.has_value())
    {
        // Only weight animation -- mesh goes directly on this node
        const auto& gltfMesh = asset.meshes[gltfNode.meshIndex.value()];
        auto object =
            loadMesh(asset, gltfMesh, baseDir, resources, assets, gltfNode.meshIndex.value());
        node.component().emplace<Mesh>(std::move(object));
        auto& mesh = std::get<Mesh>(node.component());

        if (hasWeightAnim)
        {
            mesh.morphAnimation(&la);
        }

        // Apply initial weights from glTF mesh
        std::vector<float> initialWeights(numMorphTargets, 0.0f);
        for (std::size_t w = 0; w < gltfMesh.weights.size() && w < numMorphTargets; ++w)
        {
            initialWeights[w] = gltfMesh.weights[w];
        }
        mesh.initialMorphWeights(std::move(initialWeights));
    }

    for (auto childIndex : gltfNode.children)
    {
        auto childNode = std::make_unique<Node>(nodeName(asset, asset.nodes[childIndex]));
        auto& childRef = node.addChild(std::move(childNode));
        nodeMap[childIndex] = &childRef;

        if (nodeHasAnimation(asset, childIndex))
        {
            configureAnimatedNode(asset, childIndex, childRef, baseDir, resources, assets, nodeMap,
                                  animMap);
        }
        else
        {
            loadNode(asset, childIndex, childRef, baseDir, resources, assets, nodeMap, animMap);
        }
    }
}

void GltfLoader::loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& node,
                          const std::string& baseDir, Resources& resources, Assets& assets,
                          NodeMap& nodeMap, AnimationMap& animMap)
{
    const auto& gltfNode = asset.nodes[nodeIndex];
    applyTRS(gltfNode, node);

    if (gltfNode.meshIndex.has_value())
    {
        auto object = loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()], baseDir, resources,
                               assets, gltfNode.meshIndex.value());
        node.component().emplace<Mesh>(std::move(object));
    }

    for (auto childIndex : gltfNode.children)
    {
        auto childNode = std::make_unique<Node>(nodeName(asset, asset.nodes[childIndex]));
        auto& childRef = node.addChild(std::move(childNode));
        nodeMap[childIndex] = &childRef;

        if (nodeHasAnimation(asset, childIndex))
        {
            configureAnimatedNode(asset, childIndex, childRef, baseDir, resources, assets, nodeMap,
                                  animMap);
        }
        else
        {
            loadNode(asset, childIndex, childRef, baseDir, resources, assets, nodeMap, animMap);
        }
    }
}

// ---------------------------------------------------------------------------
// Mesh loading helpers
// ---------------------------------------------------------------------------

const Texture* GltfLoader::resolveTexture(const fastgltf::Asset& asset,
                                          const fastgltf::Primitive& primitive,
                                          const std::string& texturePath, Resources& resources,
                                          Assets& assets)
{
    if (primitive.materialIndex.has_value())
    {
        const auto& gltfMat = asset.materials[primitive.materialIndex.value()];
        if (gltfMat.pbrData.baseColorTexture.has_value() && !texturePath.empty())
        {
            auto texIndex = gltfMat.pbrData.baseColorTexture.value().textureIndex;
            auto& tex = assets.texture(texIndex);
            if (!tex.loaded())
            {
                tex = Texture::load_from_file(texturePath, resources);
            }
            return &tex;
        }
    }

    return nullptr;
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
            if (texPtr != nullptr)
            {
                mat.texture(texPtr);
            }
        }
        return &mat;
    }

    auto& mat = assets.material(0);
    if (!mat.hasTexture() && texPtr != nullptr)
    {
        mat.texture(texPtr);
    }
    return &mat;
}

void GltfLoader::loadGeometry(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                              const Material* matPtr, Resources& resources, Assets& assets,
                              std::size_t geoIdx)
{
    auto& geometry = assets.geometry(geoIdx);
    if (geometry.loaded())
    {
        return;
    }

    const auto* posAttr = primitive.findAttribute("POSITION");
    const auto* normAttr = primitive.findAttribute("NORMAL");
    const auto* uvAttr = primitive.findAttribute("TEXCOORD_0");
    const auto* jointsAttr = primitive.findAttribute("JOINTS_0");
    const auto* weightsAttr = primitive.findAttribute("WEIGHTS_0");

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

    // Joint indices — may be uint8 or uint16 in glTF, stored as uint32
    std::vector<std::array<uint32_t, 4>> joints;
    if (jointsAttr != primitive.attributes.end())
    {
        const auto& jointsAccessor = asset.accessors[jointsAttr->accessorIndex];
        joints.resize(jointsAccessor.count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::u16vec4>(
            asset, jointsAccessor, [&](fastgltf::math::u16vec4 j, std::size_t idx)
            { joints[idx] = {j.x(), j.y(), j.z(), j.w()}; });
    }

    std::vector<fastgltf::math::fvec4> weights;
    if (weightsAttr != primitive.attributes.end())
    {
        const auto& weightsAccessor = asset.accessors[weightsAttr->accessorIndex];
        weights.resize(weightsAccessor.count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
            asset, weightsAccessor,
            [&](fastgltf::math::fvec4 w, std::size_t idx) { weights[idx] = w; });
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

        uint32_t j0 = 0, j1 = 0, j2 = 0, j3 = 0;
        float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.0f;
        if (i < joints.size())
        {
            j0 = joints[i][0];
            j1 = joints[i][1];
            j2 = joints[i][2];
            j3 = joints[i][3];
        }
        if (i < weights.size())
        {
            w0 = weights[i].x();
            w1 = weights[i].y();
            w2 = weights[i].z();
            w3 = weights[i].w();
        }

        verts.push_back(Vertex{pos, vertexColour, norm, u, v, j0, j1, j2, j3, w0, w1, w2, w3});
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

    // Load morph targets
    if (!primitive.targets.empty())
    {
        std::vector<std::vector<Vec3>> morphPositions;
        std::vector<std::vector<Vec3>> morphNormals;

        for (std::size_t ti = 0; ti < primitive.targets.size(); ++ti)
        {
            const auto& target = primitive.targets[ti];
            std::vector<Vec3> targetPos(positions.size());
            std::vector<Vec3> targetNorm(positions.size());

            for (const auto& attr : target)
            {
                const auto& accessor = asset.accessors[attr.accessorIndex];
                if (attr.name == "POSITION")
                {
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                        asset, accessor,
                        [&](fastgltf::math::fvec3 p, std::size_t idx)
                        { targetPos[idx] = Vec3{p.x(), p.y(), p.z()}; });
                }
                else if (attr.name == "NORMAL")
                {
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                        asset, accessor,
                        [&](fastgltf::math::fvec3 n, std::size_t idx)
                        { targetNorm[idx] = Vec3{n.x(), n.y(), n.z()}; });
                }
            }

            morphPositions.push_back(std::move(targetPos));
            morphNormals.push_back(std::move(targetNorm));
        }

        geometry.morphPositions(std::move(morphPositions));
        geometry.morphNormals(std::move(morphNormals));
    }

    geometry.load(resources);
}

Object GltfLoader::loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                            const std::string& baseDir, Resources& resources, Assets& assets,
                            std::size_t meshIndex)
{
    std::size_t geoStartIdx = 0;
    for (std::size_t m = 0; m < meshIndex; ++m)
    {
        geoStartIdx += asset.meshes[m].primitives.size();
    }

    Object object;

    for (std::size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx)
    {
        const auto& primitive = mesh.primitives[primIdx];
        auto [materialData, texturePath] = loadMaterial(asset, primitive, baseDir);

        const Texture* texPtr = resolveTexture(asset, primitive, texturePath, resources, assets);
        Material* matPtr = resolveMaterial(asset, primitive, materialData, texPtr, assets);

        std::size_t geoIdx = geoStartIdx + primIdx;
        loadGeometry(asset, primitive, matPtr, resources, assets, geoIdx);
        object.addGeometry(assets.geometry(geoIdx));
    }

    object.load(resources);
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
        {
            continue;
        }

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

Animation::Interpolation GltfLoader::mapInterpolation(fastgltf::AnimationInterpolation m)
{
    switch (m)
    {
        case fastgltf::AnimationInterpolation::Step:
            return Animation::Interpolation::Step;
        case fastgltf::AnimationInterpolation::CubicSpline:
            return Animation::Interpolation::CubicSpline;
        case fastgltf::AnimationInterpolation::Linear:
        default:
            return Animation::Interpolation::Linear;
    }
}

namespace
{

// glTF CUBICSPLINE output layout: per keyframe, three elements packed as
// [in_tangent, value, out_tangent]. For non-CubicSpline modes there is a single
// value per keyframe. Returns the stride (3 for CubicSpline, 1 otherwise).
constexpr std::size_t outputStride(Animation::Interpolation m) noexcept
{
    return m == Animation::Interpolation::CubicSpline ? 3 : 1;
}

} // namespace

void GltfLoader::loadRotationChannel(const fastgltf::Asset& asset,
                                     const fastgltf::AnimationSampler& sampler, Animation& anim)
{
    const auto interp = mapInterpolation(sampler.interpolation);
    const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

    std::vector<float> times(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor,
                                              [&](float t, std::size_t idx) { times[idx] = t; });

    std::vector<fastgltf::math::fvec4> quats(outputAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
        asset, outputAccessor, [&](fastgltf::math::fvec4 q, std::size_t idx) { quats[idx] = q; });

    const std::size_t stride = outputStride(interp);
    const std::size_t valueOffset = (interp == Animation::Interpolation::CubicSpline) ? 1 : 0;

    std::vector<Animation::RotationKeyframe> kf;
    kf.reserve(times.size());
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        std::size_t vi = i * stride + valueOffset;
        if (vi >= quats.size()) break;
        kf.push_back({times[i], quats[vi].x(), quats[vi].y(), quats[vi].z(), quats[vi].w()});
    }

    anim.rotationKeyframes(std::move(kf));
    anim.rotationInterpolation(interp);

    if (interp == Animation::Interpolation::CubicSpline)
    {
        std::vector<Quaternion> in;
        std::vector<Quaternion> out;
        in.reserve(times.size());
        out.reserve(times.size());
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            std::size_t inIdx = i * 3;
            std::size_t outIdx = i * 3 + 2;
            if (outIdx >= quats.size()) break;
            in.push_back(Quaternion{quats[inIdx].x(), quats[inIdx].y(), quats[inIdx].z(),
                                    quats[inIdx].w()});
            out.push_back(Quaternion{quats[outIdx].x(), quats[outIdx].y(), quats[outIdx].z(),
                                     quats[outIdx].w()});
        }
        anim.rotationTangents(std::move(in), std::move(out));
    }
}

void GltfLoader::loadTranslationChannel(const fastgltf::Asset& asset,
                                        const fastgltf::AnimationSampler& sampler, Animation& anim)
{
    const auto interp = mapInterpolation(sampler.interpolation);
    const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

    std::vector<float> times(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor,
                                              [&](float t, std::size_t idx) { times[idx] = t; });

    std::vector<fastgltf::math::fvec3> positions(outputAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, outputAccessor,
        [&](fastgltf::math::fvec3 p, std::size_t idx) { positions[idx] = p; });

    const std::size_t stride = outputStride(interp);
    const std::size_t valueOffset = (interp == Animation::Interpolation::CubicSpline) ? 1 : 0;

    std::vector<Animation::TranslationKeyframe> kf;
    kf.reserve(times.size());
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        std::size_t vi = i * stride + valueOffset;
        if (vi >= positions.size()) break;
        kf.push_back(
            {times[i], Vec3{positions[vi].x(), positions[vi].y(), positions[vi].z()}});
    }

    anim.translationKeyframes(std::move(kf));
    anim.translationInterpolation(interp);

    if (interp == Animation::Interpolation::CubicSpline)
    {
        std::vector<Vec3> in;
        std::vector<Vec3> out;
        in.reserve(times.size());
        out.reserve(times.size());
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            std::size_t inIdx = i * 3;
            std::size_t outIdx = i * 3 + 2;
            if (outIdx >= positions.size()) break;
            in.push_back(Vec3{positions[inIdx].x(), positions[inIdx].y(), positions[inIdx].z()});
            out.push_back(
                Vec3{positions[outIdx].x(), positions[outIdx].y(), positions[outIdx].z()});
        }
        anim.translationTangents(std::move(in), std::move(out));
    }
}

void GltfLoader::loadScaleChannel(const fastgltf::Asset& asset,
                                  const fastgltf::AnimationSampler& sampler, Animation& anim)
{
    const auto interp = mapInterpolation(sampler.interpolation);
    const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

    std::vector<float> times(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor,
                                              [&](float t, std::size_t idx) { times[idx] = t; });

    std::vector<fastgltf::math::fvec3> scales(outputAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, outputAccessor, [&](fastgltf::math::fvec3 s, std::size_t idx) { scales[idx] = s; });

    const std::size_t stride = outputStride(interp);
    const std::size_t valueOffset = (interp == Animation::Interpolation::CubicSpline) ? 1 : 0;

    std::vector<Animation::ScaleKeyframe> kf;
    kf.reserve(times.size());
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        std::size_t vi = i * stride + valueOffset;
        if (vi >= scales.size()) break;
        kf.push_back({times[i], Vec3{scales[vi].x(), scales[vi].y(), scales[vi].z()}});
    }

    anim.scaleKeyframes(std::move(kf));
    anim.scaleInterpolation(interp);

    if (interp == Animation::Interpolation::CubicSpline)
    {
        std::vector<Vec3> in;
        std::vector<Vec3> out;
        in.reserve(times.size());
        out.reserve(times.size());
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            std::size_t inIdx = i * 3;
            std::size_t outIdx = i * 3 + 2;
            if (outIdx >= scales.size()) break;
            in.push_back(Vec3{scales[inIdx].x(), scales[inIdx].y(), scales[inIdx].z()});
            out.push_back(Vec3{scales[outIdx].x(), scales[outIdx].y(), scales[outIdx].z()});
        }
        anim.scaleTangents(std::move(in), std::move(out));
    }
}

void GltfLoader::loadWeightChannel(const fastgltf::Asset& asset,
                                   const fastgltf::AnimationSampler& sampler, Animation& anim,
                                   std::size_t numTargets)
{
    const auto interp = mapInterpolation(sampler.interpolation);
    const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
    const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

    std::vector<float> times(inputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(asset, inputAccessor,
                                              [&](float t, std::size_t idx) { times[idx] = t; });

    // Output is a flat array of scalars. Linear/Step: numKeyframes * numTargets.
    // CubicSpline: numKeyframes * 3 * numTargets (in_tan, value, out_tan per key).
    std::vector<float> allWeights(outputAccessor.count);
    fastgltf::iterateAccessorWithIndex<float>(
        asset, outputAccessor, [&](float w, std::size_t idx) { allWeights[idx] = w; });

    const std::size_t stride = outputStride(interp);
    const std::size_t valueOffset = (interp == Animation::Interpolation::CubicSpline) ? 1 : 0;

    std::vector<Animation::WeightKeyframe> kf;
    kf.reserve(times.size());
    for (std::size_t i = 0; i < times.size(); ++i)
    {
        Animation::WeightKeyframe wkf;
        wkf.time = times[i];
        wkf.weights.resize(numTargets, 0.0f);
        std::size_t base = (i * stride + valueOffset) * numTargets;
        for (std::size_t w = 0; w < numTargets; ++w)
        {
            std::size_t idx = base + w;
            if (idx < allWeights.size())
            {
                wkf.weights[w] = allWeights[idx];
            }
        }
        kf.push_back(std::move(wkf));
    }

    anim.weightKeyframes(std::move(kf));
    anim.weightInterpolation(interp);

    if (interp == Animation::Interpolation::CubicSpline)
    {
        std::vector<std::vector<float>> in;
        std::vector<std::vector<float>> out;
        in.reserve(times.size());
        out.reserve(times.size());
        for (std::size_t i = 0; i < times.size(); ++i)
        {
            std::vector<float> inV(numTargets, 0.0f);
            std::vector<float> outV(numTargets, 0.0f);
            std::size_t inBase = (i * 3) * numTargets;
            std::size_t outBase = (i * 3 + 2) * numTargets;
            for (std::size_t w = 0; w < numTargets; ++w)
            {
                if (inBase + w < allWeights.size()) inV[w] = allWeights[inBase + w];
                if (outBase + w < allWeights.size()) outV[w] = allWeights[outBase + w];
            }
            in.push_back(std::move(inV));
            out.push_back(std::move(outV));
        }
        anim.weightTangents(std::move(in), std::move(out));
    }
}

void GltfLoader::loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                               Animation& la, std::size_t numMorphTargets)
{
    float sharedDuration = computeSharedDuration(asset, nodeIndex);

    for (const auto& anim : asset.animations)
    {
        for (const auto& channel : anim.channels)
        {
            if (!channel.nodeIndex.has_value() || channel.nodeIndex.value() != nodeIndex)
            {
                continue;
            }

            const auto& sampler = anim.samplers[channel.samplerIndex];

            if (channel.path == fastgltf::AnimationPath::Rotation)
            {
                loadRotationChannel(asset, sampler, la);
            }
            else if (channel.path == fastgltf::AnimationPath::Translation)
            {
                loadTranslationChannel(asset, sampler, la);
            }
            else if (channel.path == fastgltf::AnimationPath::Scale)
            {
                loadScaleChannel(asset, sampler, la);
            }
            else if (channel.path == fastgltf::AnimationPath::Weights && numMorphTargets > 0)
            {
                loadWeightChannel(asset, sampler, la, numMorphTargets);
            }
        }
    }

    la.duration(sharedDuration);
}

// ---------------------------------------------------------------------------
// Skin loading helpers
// ---------------------------------------------------------------------------

void GltfLoader::loadSkin(const fastgltf::Asset& asset, std::size_t skinIndex,
                          const NodeMap& nodeMap, Assets& assets)
{
    const auto& gltfSkin = asset.skins[skinIndex];
    auto& skin = assets.skin(skinIndex);

    if (!gltfSkin.name.empty())
    {
        skin.name(std::string(gltfSkin.name));
    }

    // Read inverse bind matrices (one per joint)
    std::vector<Mat4> inverseBindMatrices(gltfSkin.joints.size(), Mat4::identity());
    if (gltfSkin.inverseBindMatrices.has_value())
    {
        const auto& accessor = asset.accessors[gltfSkin.inverseBindMatrices.value()];
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fmat4x4>(
            asset, accessor,
            [&](const fastgltf::math::fmat4x4& m, std::size_t idx)
            {
                Mat4 mat;
                for (int col = 0; col < 4; ++col)
                {
                    for (int row = 0; row < 4; ++row)
                    {
                        mat[row, col] = m.col(col)[row];
                    }
                }
                inverseBindMatrices[idx] = mat;
            });
    }

    for (std::size_t i = 0; i < gltfSkin.joints.size(); ++i)
    {
        auto jointNodeIndex = gltfSkin.joints[i];
        auto it = nodeMap.find(jointNodeIndex);
        if (it == nodeMap.end())
        {
            throw std::runtime_error("Skin joint references unknown node index " +
                                     std::to_string(jointNodeIndex));
        }
        skin.addJoint(it->second, inverseBindMatrices[i]);
    }
}

void GltfLoader::applySkins(const fastgltf::Asset& asset, const NodeMap& nodeMap, Assets& assets)
{
    for (const auto& [nodeIndex, nodePtr] : nodeMap)
    {
        const auto& gltfNode = asset.nodes[nodeIndex];
        if (!gltfNode.skinIndex.has_value())
        {
            continue;
        }

        auto skinIndex = gltfNode.skinIndex.value();
        if (assets.skin(skinIndex).empty())
        {
            loadSkin(asset, skinIndex, nodeMap, assets);
        }

        auto* comp = std::get_if<Mesh>(&nodePtr->component());
        if (comp != nullptr)
        {
            comp->skin(&assets.skin(skinIndex));
        }
    }
}

} // namespace fire_engine
