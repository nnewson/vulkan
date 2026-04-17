#include <fire_engine/graphics/object.hpp>

#include <cstring>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/skin.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/render/resources.hpp>
#include <fire_engine/render/ubo.hpp>

namespace fire_engine
{

void Object::addGeometry(const Geometry& geometry)
{
    auto& binding = bindings_.emplace_back();
    binding.geometry = &geometry;
}

void Object::load(Resources& resources)
{
    // Create shared uniform buffers (model/view/proj)
    auto uniformSet = resources.createMappedUniformBuffers(sizeof(UniformBufferObject));
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uniformMapped_[i] = uniformSet.mapped[i];
    }

    Resources::ObjectDescriptorRequest req;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        req.uniformBufs[i] = uniformSet.buffers[i];
    }

    for (auto& binding : bindings_)
    {
        Resources::GeometryDescriptorInfo geoInfo;

        // Material buffers
        MaterialUBO matUbo = toMaterialUBO(binding.geometry->material());
        auto matSet = resources.createMappedUniformBuffers(sizeof(MaterialUBO));
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            binding.materialMapped[i] = matSet.mapped[i];
            geoInfo.materialBufs[i] = matSet.buffers[i];
            std::memcpy(matSet.mapped[i], &matUbo, sizeof(matUbo));
        }

        // Skin buffers
        SkinUBO skinUbo{};
        for (std::size_t j = 0; j < MAX_JOINTS; ++j)
        {
            skinUbo.joints[j] = Mat4::identity();
        }
        auto skinSet = resources.createMappedUniformBuffers(sizeof(SkinUBO));
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            binding.skinMapped[i] = skinSet.mapped[i];
            geoInfo.skinBufs[i] = skinSet.buffers[i];
            std::memcpy(skinSet.mapped[i], &skinUbo, sizeof(skinUbo));
        }

        // Morph UBO buffers
        MorphUBO morphUbo{};
        auto morphUboSet = resources.createMappedUniformBuffers(sizeof(MorphUBO));
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            binding.morphUboMapped[i] = morphUboSet.mapped[i];
            geoInfo.morphUboBufs[i] = morphUboSet.buffers[i];
            std::memcpy(morphUboSet.mapped[i], &morphUbo, sizeof(morphUbo));
        }

        // Morph SSBO
        auto numTargets = binding.geometry->morphTargetCount();
        auto numVerts = binding.geometry->vertices().size();

        if (numTargets == 0 || numVerts == 0)
        {
            // Minimal dummy SSBO
            float zeros[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            auto ssboSet = resources.createMappedStorageBuffer(sizeof(zeros), zeros);
            geoInfo.morphSsbo = ssboSet.buffers[0];
            geoInfo.morphSsboSize = 0;
        }
        else
        {
            // Pack position and normal deltas as vec4 (xyz + padding)
            std::size_t totalEntries = numTargets * numVerts * 2;
            std::size_t ssboSize = totalEntries * sizeof(float) * 4;
            std::vector<float> ssboData(totalEntries * 4, 0.0f);
            float* dst = ssboData.data();

            const auto& morphPositions = binding.geometry->morphPositions();
            const auto& morphNormals = binding.geometry->morphNormals();

            // Write position deltas
            for (std::size_t t = 0; t < numTargets; ++t)
            {
                for (std::size_t v = 0; v < numVerts; ++v)
                {
                    const auto& pos =
                        (t < morphPositions.size() && v < morphPositions[t].size())
                            ? morphPositions[t][v]
                            : Vec3{};
                    *dst++ = pos.x();
                    *dst++ = pos.y();
                    *dst++ = pos.z();
                    *dst++ = 0.0f;
                }
            }

            // Write normal deltas
            for (std::size_t t = 0; t < numTargets; ++t)
            {
                for (std::size_t v = 0; v < numVerts; ++v)
                {
                    const auto& norm =
                        (t < morphNormals.size() && v < morphNormals[t].size())
                            ? morphNormals[t][v]
                            : Vec3{};
                    *dst++ = norm.x();
                    *dst++ = norm.y();
                    *dst++ = norm.z();
                    *dst++ = 0.0f;
                }
            }

            auto ssboSet = resources.createMappedStorageBuffer(ssboSize, ssboData.data());
            geoInfo.morphSsbo = ssboSet.buffers[0];
            geoInfo.morphSsboSize = ssboSize;
        }

        // Texture handle — use a 1x1 white dummy when material has no texture
        if (binding.geometry->material().hasTexture())
        {
            geoInfo.texture = binding.geometry->material().texture().handle();
        }
        else
        {
            static const uint8_t white[] = {255, 255, 255, 255};
            geoInfo.texture = resources.createTexture(white, 1, 1);
        }

        req.geometries.push_back(geoInfo);
    }

    auto descResult = resources.createObjectDescriptors(req);
    for (std::size_t g = 0; g < bindings_.size(); ++g)
    {
        bindings_[g].descSets = descResult.descSets[g];
    }
}

MaterialUBO Object::toMaterialUBO(const Material& mat)
{
    MaterialUBO ubo{};
    ubo.ambient[0] = mat.ambient().r();
    ubo.ambient[1] = mat.ambient().g();
    ubo.ambient[2] = mat.ambient().b();
    ubo.diffuse[0] = mat.diffuse().r();
    ubo.diffuse[1] = mat.diffuse().g();
    ubo.diffuse[2] = mat.diffuse().b();
    ubo.specular[0] = mat.specular().r();
    ubo.specular[1] = mat.specular().g();
    ubo.specular[2] = mat.specular().b();
    ubo.emissive[0] = mat.emissive().r();
    ubo.emissive[1] = mat.emissive().g();
    ubo.emissive[2] = mat.emissive().b();
    ubo.shininess = mat.shininess();
    ubo.ior = mat.ior();
    ubo.transparency = mat.transparency();
    ubo.illum = mat.illum();
    ubo.roughness = mat.roughness();
    ubo.metallic = mat.metallic();
    ubo.sheen = mat.sheen();
    ubo.clearcoat = mat.clearcoat();
    ubo.clearcoatRoughness = mat.clearcoatRoughness();
    ubo.anisotropy = mat.anisotropy();
    ubo.anisotropyRotation = mat.anisotropyRotation();
    ubo.hasTexture = mat.hasTexture() ? 1 : 0;
    return ubo;
}

void Object::updateSkin()
{
    if (skin_ != nullptr && !skin_->empty())
    {
        skin_->updateJointMatrices();
    }
}

std::vector<DrawCommand> Object::render(const FrameInfo& frame, const Mat4& world)
{
    // Write shared UBO
    UniformBufferObject ubo{};
    ubo.model = world;
    ubo.view = Mat4::lookAt(frame.cameraPosition, frame.cameraTarget, {0, 1, 0});
    float aspect =
        static_cast<float>(frame.viewportWidth) / static_cast<float>(frame.viewportHeight);
    ubo.proj = Mat4::perspective(45.0f * deg_to_rad, aspect, 0.1f, 1000.0f);
    ubo.cameraPos[0] = frame.cameraPosition.x();
    ubo.cameraPos[1] = frame.cameraPosition.y();
    ubo.cameraPos[2] = frame.cameraPosition.z();
    ubo.cameraPos[3] = 0.0f;
    ubo.hasSkin = (skin_ != nullptr && !skin_->empty()) ? 1 : 0;
    std::memcpy(uniformMapped_[frame.currentFrame], &ubo, sizeof(ubo));

    // Upload cached joint matrices if skinned
    if (ubo.hasSkin == 1)
    {
        const auto& jointMatrices = skin_->cachedJointMatrices();
        SkinUBO skinUbo{};
        for (std::size_t j = 0; j < jointMatrices.size() && j < MAX_JOINTS; ++j)
        {
            skinUbo.joints[j] = jointMatrices[j];
        }
        for (auto& binding : bindings_)
        {
            std::memcpy(binding.skinMapped[frame.currentFrame], &skinUbo, sizeof(skinUbo));
        }
    }

    // Upload morph weights
    for (auto& binding : bindings_)
    {
        auto numTargets = binding.geometry->morphTargetCount();
        MorphUBO morphUbo{};
        if (numTargets > 0 && !morphWeights_.empty())
        {
            morphUbo.hasMorph = 1;
            morphUbo.morphTargetCount = static_cast<int>(numTargets);
            morphUbo.vertexCount = static_cast<int>(binding.geometry->vertices().size());
            for (std::size_t w = 0;
                 w < morphWeights_.size() && w < static_cast<std::size_t>(MAX_MORPH_TARGETS); ++w)
            {
                morphUbo.weights[w] = morphWeights_[w];
            }
        }
        std::memcpy(binding.morphUboMapped[frame.currentFrame], &morphUbo, sizeof(morphUbo));
    }

    // Build draw commands
    std::vector<DrawCommand> commands;
    commands.reserve(bindings_.size());
    for (const auto& binding : bindings_)
    {
        DrawCommand cmd;
        cmd.vertexBuffer = binding.geometry->vertexBuffer();
        cmd.indexBuffer = binding.geometry->indexBuffer();
        cmd.indexCount = binding.geometry->indexCount();
        cmd.descriptorSet = binding.descSets[frame.currentFrame];
        cmd.pipeline = frame.pipeline;
        commands.push_back(cmd);
    }
    return commands;
}

} // namespace fire_engine
