#include <fire_engine/core/gltf_loader.hpp>

#include <cmath>
#include <filesystem>
#include <stdexcept>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/frame.hpp>
#include <fire_engine/renderer/pipeline.hpp>
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

} // namespace

void GltfLoader::loadScene(const std::string& path, SceneGraph& scene, const Device& device,
                           const Pipeline& pipeline, Frame& frame)
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

            // Apply the node's transform to the animator node
            const auto& gltfNode = asset.nodes[nodeIndex];
            if (auto* trs = std::get_if<fastgltf::TRS>(&gltfNode.transform))
            {
                rootRef.transform().position(
                    {trs->translation.x(), trs->translation.y(), trs->translation.z()});
                rootRef.transform().rotation(quaternionToEuler(
                    trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()));
                rootRef.transform().scale({trs->scale.x(), trs->scale.y(), trs->scale.z()});
            }

            auto& animRef = rootRef.addChild(std::move(animNode));

            // Load mesh as child of animator
            if (gltfNode.meshIndex.has_value())
            {
                auto meshNode = std::make_unique<Node>(std::string(gltfNode.name) + "_Mesh");
                meshNode->component().emplace<Mesh>();
                auto& meshRef = animRef.addChild(std::move(meshNode));
                loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()], meshRef, baseDir.string(),
                         device, pipeline, frame);
            }

            // Recurse into child nodes
            for (auto childIndex : gltfNode.children)
            {
                loadNode(asset, childIndex, animRef, baseDir.string(), device, pipeline, frame);
            }
        }
        else
        {
            loadNode(asset, nodeIndex, rootRef, baseDir.string(), device, pipeline, frame);
        }
    }
}

void GltfLoader::loadNode(const fastgltf::Asset& asset, std::size_t nodeIndex, Node& parentNode,
                          const std::string& baseDir, const Device& device,
                          const Pipeline& pipeline, Frame& frame)
{
    const auto& gltfNode = asset.nodes[nodeIndex];

    // Apply transform
    if (auto* trs = std::get_if<fastgltf::TRS>(&gltfNode.transform))
    {
        parentNode.transform().position(
            {trs->translation.x(), trs->translation.y(), trs->translation.z()});
        parentNode.transform().rotation(quaternionToEuler(trs->rotation.x(), trs->rotation.y(),
                                                          trs->rotation.z(), trs->rotation.w()));
        parentNode.transform().scale({trs->scale.x(), trs->scale.y(), trs->scale.z()});
    }

    // Load mesh if present
    if (gltfNode.meshIndex.has_value())
    {
        parentNode.component().emplace<Mesh>();
        loadMesh(asset, asset.meshes[gltfNode.meshIndex.value()], parentNode, baseDir, device,
                 pipeline, frame);
    }

    // Recurse into children
    for (auto childIndex : gltfNode.children)
    {
        auto childNode = std::make_unique<Node>(std::string(asset.nodes[childIndex].name));
        auto& childRef = parentNode.addChild(std::move(childNode));

        if (nodeHasAnimation(asset, childIndex))
        {
            auto animNode =
                std::make_unique<Node>(std::string(asset.nodes[childIndex].name) + "_Animator");
            animNode->component().emplace<Animator>();
            auto& animRef = childRef.addChild(std::move(animNode));

            const auto& childGltfNode = asset.nodes[childIndex];
            if (childGltfNode.meshIndex.has_value())
            {
                auto meshNode = std::make_unique<Node>(std::string(childGltfNode.name) + "_Mesh");
                meshNode->component().emplace<Mesh>();
                auto& meshRef = animRef.addChild(std::move(meshNode));
                loadMesh(asset, asset.meshes[childGltfNode.meshIndex.value()], meshRef, baseDir,
                         device, pipeline, frame);
            }

            for (auto grandchildIndex : childGltfNode.children)
            {
                loadNode(asset, grandchildIndex, animRef, baseDir, device, pipeline, frame);
            }
        }
        else
        {
            loadNode(asset, childIndex, childRef, baseDir, device, pipeline, frame);
        }
    }
}

void GltfLoader::loadMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, Node& node,
                          const std::string& baseDir, const Device& device,
                          const Pipeline& pipeline, Frame& frame)
{
    for (const auto& primitive : mesh.primitives)
    {
        Geometry renderData;
        Material material;
        std::string texturePath;

        // Extract material properties
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

        Colour3 vertexColour = material.diffuse();

        // Build vertices
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
        renderData.vertices(std::move(verts));

        // Read indices
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
            // No indices — generate sequential indices
            for (std::size_t i = 0; i < positions.size(); ++i)
            {
                idxs.push_back(static_cast<uint16_t>(i));
            }
        }
        renderData.indices(std::move(idxs));

        auto& meshComponent = std::get<Mesh>(node.component());
        meshComponent.load(renderData, material, texturePath, device, pipeline, frame);
    }
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

} // namespace fire_engine
