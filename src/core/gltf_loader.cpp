#include "fire_engine/renderer/renderer.hpp"
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

namespace
{

Vec3 quaternionToEuler(float qx, float qy, float qz, float qw)
{
    // Roll (Z)
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    // Pitch (X)
    float sinp = 2.0f * (qw * qy - qz * qx);
    float pitch =
        (std::abs(sinp) >= 1.0f) ? std::copysign(3.14159265f / 2.0f, sinp) : std::asin(sinp);

    // Yaw (Y)
    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return {pitch, yaw, roll};
}

void applyTRS(const fastgltf::Node& gltfNode, Node& node)
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

} // namespace

void GltfLoader::loadScene(const std::string& path, SceneGraph& scene, const Renderer& renderer,
                          Assets& assets)
{
    auto gltfPath = std::filesystem::path(path);
    auto baseDir = gltfPath.parent_path();

    fastgltf::Parser parser;
    auto dataResult = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    if (dataResult.error() != fastgltf::Error::None)
    {
        throw std::runtime_error("Failed to read glTF file: " +
                                 std::string(fastgltf::getErrorMessage(dataResult.error())));
    }
    auto& data = dataResult.get();

    auto options = fastgltf::Options::LoadExternalBuffers;
    auto result = parser.loadGltf(data, baseDir, options);
    if (result.error() != fastgltf::Error::None)
    {
        throw std::runtime_error("Failed to load glTF: " +
                                 std::string(fastgltf::getErrorMessage(result.error())));
    }

    auto& asset = result.get();

    // Pre-size asset vectors to keep pointers stable (ensure at least 1 slot for defaults)
    assets.resizeTextures(std::max<std::size_t>(asset.textures.size(), 1));
    assets.resizeMaterials(std::max<std::size_t>(asset.materials.size(), 1));
    assets.resizeGeometries(asset.meshes.size());

    std::size_t sceneIndex = asset.defaultScene.has_value() ? asset.defaultScene.value() : 0;
    if (sceneIndex >= asset.scenes.size())
    {
        throw std::runtime_error("glTF scene index out of range");
    }

    const auto& gltfScene = asset.scenes[sceneIndex];
    for (auto nodeIndex : gltfScene.nodeIndices)
    {
        // Create a dummy root node for scene-level children
        auto rootNode = std::make_unique<Node>(std::string(asset.nodes[nodeIndex].name));
        auto& rootRef = scene.addNode(std::move(rootNode));

        // If this node has an animation, wrap it with an Animator
        if (nodeHasAnimation(asset, nodeIndex))
        {
            auto animNode =
                std::make_unique<Node>(std::string(asset.nodes[nodeIndex].name) + "_Animator");
            animNode->component().emplace<Animator>();

            // Apply the node's static TRS to the root node
            const auto& gltfNode = asset.nodes[nodeIndex];
            applyTRS(gltfNode, rootRef);

            auto& animRef = rootRef.addChild(std::move(animNode));
            loadAnimation(asset, nodeIndex, std::get<Animator>(animRef.component()));

            // Load mesh as child of animator
            if (gltfNode.meshIndex.has_value())
            {
                auto meshNode = std::make_unique<Node>(std::string(gltfNode.name) + "_Mesh");
                auto& meshRef = animRef.addChild(std::move(meshNode));
                auto object = loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()],
                                       baseDir.string(), renderer, assets,
                                       gltfNode.meshIndex.value());
                meshRef.component().emplace<Mesh>(std::move(object));
            }

            // Recurse into child nodes
            for (auto childIndex : gltfNode.children)
            {
                loadNode(asset, childIndex, animRef, baseDir.string(), renderer, assets);
            }
        }
        else
        {
            loadNode(asset, nodeIndex, rootRef, baseDir.string(), renderer, assets);
        }
    }
}

void GltfLoader::loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& node,
                          const std::string& baseDir, const Renderer& renderer, Assets& assets)
{
    const auto& gltfNode = asset.nodes[nodeIndex];

    // Apply this node's static TRS to its own Node
    applyTRS(gltfNode, node);

    // Load mesh if present
    if (gltfNode.meshIndex.has_value())
    {
        auto object = loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()], baseDir, renderer,
                               assets, gltfNode.meshIndex.value());
        node.component().emplace<Mesh>(std::move(object));
    }

    // Recurse into children
    for (auto childIndex : gltfNode.children)
    {
        auto childNode = std::make_unique<Node>(std::string(asset.nodes[childIndex].name));
        auto& childRef = node.addChild(std::move(childNode));

        if (nodeHasAnimation(asset, childIndex))
        {
            const auto& childGltfNode = asset.nodes[childIndex];

            // Apply the child's static TRS to its own node before inserting the animator.
            // TODO: glTF spec says animated channels *replace* the corresponding static
            // TRS component. We currently apply the full static TRS here and then let
            // the Animator compose its sampled matrix on top, which double-applies if an
            // asset has both a non-identity static translation/rotation and an animation
            // on the same component. Fix by skipping animated channels when applying.
            applyTRS(childGltfNode, childRef);

            auto animNode = std::make_unique<Node>(std::string(childGltfNode.name) + "_Animator");
            animNode->component().emplace<Animator>();
            auto& animRef = childRef.addChild(std::move(animNode));
            loadAnimation(asset, childIndex, std::get<Animator>(animRef.component()));

            if (childGltfNode.meshIndex.has_value())
            {
                auto meshNode = std::make_unique<Node>(std::string(childGltfNode.name) + "_Mesh");
                auto& meshRef = animRef.addChild(std::move(meshNode));
                auto object =
                    loadMesh(asset, asset.meshes[childGltfNode.meshIndex.value()], baseDir,
                             renderer, assets, childGltfNode.meshIndex.value());
                meshRef.component().emplace<Mesh>(std::move(object));
            }

            for (auto grandchildIndex : childGltfNode.children)
            {
                loadNode(asset, grandchildIndex, animRef, baseDir, renderer, assets);
            }
        }
        else
        {
            loadNode(asset, childIndex, childRef, baseDir, renderer, assets);
        }
    }
}

Object GltfLoader::loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh,
                            const std::string& baseDir, const Renderer& renderer, Assets& assets,
                            std::size_t meshIndex)
{
    for (const auto& primitive : mesh.primitives)
    {
        auto [materialData, texturePath] = loadMaterial(asset, primitive, baseDir);

        // Load or reuse texture in Assets
        const Texture* texPtr = nullptr;
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
                    tex = Texture::load_from_file(texPath, device.device(), device.physicalDevice(),
                                                  renderer.frame().commandPool(),
                                                  device.graphicsQueue());
                }
                texPtr = &tex;
            }
        }
        if (!texPtr)
        {
            // Load default texture into slot 0 if no texture assigned
            auto& defaultTex = assets.texture(0);
            if (defaultTex.view() == vk::ImageView{})
            {
                auto& device = renderer.device();
                defaultTex =
                    Texture::load_from_file("default.png", device.device(), device.physicalDevice(),
                                            renderer.frame().commandPool(), device.graphicsQueue());
            }
            texPtr = &defaultTex;
        }

        // Load or reuse material in Assets
        Material* matPtr = nullptr;
        if (primitive.materialIndex.has_value())
        {
            auto matIndex = primitive.materialIndex.value();
            auto& mat = assets.material(matIndex);
            if (!mat.hasTexture())
            {
                mat = materialData;
                mat.texture(texPtr);
            }
            matPtr = &mat;
        }
        else
        {
            // No material — use slot 0 with default values
            auto& mat = assets.material(0);
            if (!mat.hasTexture())
            {
                mat.texture(texPtr);
            }
            matPtr = &mat;
        }

        // Load or reuse geometry in Assets
        auto& geometry = assets.geometry(meshIndex);
        if (!geometry.loaded())
        {
            // Read vertex attributes
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
                Vec3 norm =
                    (i < normals.size())
                        ? Vec3{normals[i].x(), normals[i].y(), normals[i].z()}
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
                fastgltf::iterateAccessor<std::uint32_t>(
                    asset, indexAccessor,
                    [&](std::uint32_t idx) { idxs.push_back(static_cast<uint16_t>(idx)); });
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

        // Create per-instance Object referencing the shared Geometry
        Object object;
        object.load(geometry, renderer);
        return object;
    }

    // Fallback (should not reach here if mesh has primitives)
    return Object{};
}

std::pair<Material, std::string>
GltfLoader::loadMaterial(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
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

void GltfLoader::loadAnimation(const fastgltf::Asset& asset, std::size_t nodeIndex,
                               Animator& animator)
{
    auto& la = animator.animation();

    // Per the glTF spec (and the BoxAnimated "Common Problems" note), channels of
    // the same animation must stay in sync even if they have different end times —
    // the shorter channel clamps to its last key while the animation keeps running
    // until the longest channel completes. To do that, every LinearAnimation that
    // participates in a given glTF animation must loop on the *same* duration: the
    // max input time across ALL of that animation's channels, not just the ones
    // targeting this node.
    float sharedDuration = 0.0f;

    for (const auto& anim : asset.animations)
    {
        // Does this animation touch our node at all?
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

        // Fold every channel's input range (including channels targeting other
        // nodes) into the shared duration so all participants loop in lockstep.
        for (const auto& channel : anim.channels)
        {
            const auto& sampler = anim.samplers[channel.samplerIndex];
            const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
            fastgltf::iterateAccessor<float>(asset, inputAccessor, [&](float t)
                                             { sharedDuration = std::max(sharedDuration, t); });
        }

        for (const auto& channel : anim.channels)
        {
            if (!channel.nodeIndex.has_value() || channel.nodeIndex.value() != nodeIndex)
            {
                continue;
            }

            const auto& sampler = anim.samplers[channel.samplerIndex];
            const auto& inputAccessor = asset.accessors[sampler.inputAccessor];
            const auto& outputAccessor = asset.accessors[sampler.outputAccessor];

            // Read keyframe times (shared by all channel types)
            std::vector<float> times(inputAccessor.count);
            fastgltf::iterateAccessorWithIndex<float>(
                asset, inputAccessor, [&](float t, std::size_t idx) { times[idx] = t; });

            if (channel.path == fastgltf::AnimationPath::Rotation)
            {
                std::vector<fastgltf::math::fvec4> quats(outputAccessor.count);
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                    asset, outputAccessor,
                    [&](fastgltf::math::fvec4 q, std::size_t idx) { quats[idx] = q; });

                std::vector<LinearAnimation::RotationKeyframe> kf;
                std::size_t count = std::min(times.size(), quats.size());
                kf.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    kf.push_back(
                        {times[i], quats[i].x(), quats[i].y(), quats[i].z(), quats[i].w()});
                }
                la.rotationKeyframes(std::move(kf));
            }
            else if (channel.path == fastgltf::AnimationPath::Translation)
            {
                std::vector<fastgltf::math::fvec3> positions(outputAccessor.count);
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, outputAccessor,
                    [&](fastgltf::math::fvec3 p, std::size_t idx) { positions[idx] = p; });

                std::vector<LinearAnimation::TranslationKeyframe> kf;
                std::size_t count = std::min(times.size(), positions.size());
                kf.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    kf.push_back(
                        {times[i], Vec3{positions[i].x(), positions[i].y(), positions[i].z()}});
                }
                la.translationKeyframes(std::move(kf));
            }
            // Scale channels and morph weights are not supported; silently skip.
        }
    }

    la.duration(sharedDuration);
}

} // namespace fire_engine
