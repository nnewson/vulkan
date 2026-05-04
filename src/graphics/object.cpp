#include <fire_engine/graphics/object.hpp>

#include <cmath>
#include <cstring>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/skin.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/render/constants.hpp>
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
    const auto& lightBufs = resources.lightBuffers();
    req.shadowMap = resources.shadowMap();
    req.irradianceMap = resources.irradianceMap();
    req.prefilteredMap = resources.prefilteredMap();
    req.brdfLut = resources.brdfLut();
    req.sceneColor = resources.sceneColor();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        req.uniformBufs[i] = uniformSet.buffers[i];
        req.lightBufs[i] = lightBufs[i];
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
            // Pack pos/normal/tangent deltas as vec4 (xyz + 0). Layout:
            // [pos0..N, norm0..N, tang0..N]. Tangents may be all-zeros when
            // the source asset doesn't ship morph TANGENT data.
            std::size_t totalEntries = numTargets * numVerts * 3;
            std::size_t ssboSize = totalEntries * sizeof(float) * 4;
            std::vector<float> ssboData(totalEntries * 4, 0.0f);
            float* dst = ssboData.data();

            const auto& morphPositions = binding.geometry->morphPositions();
            const auto& morphNormals = binding.geometry->morphNormals();
            const auto& morphTangents = binding.geometry->morphTangents();

            auto writeDeltas = [&](const std::vector<std::vector<Vec3>>& src)
            {
                for (std::size_t t = 0; t < numTargets; ++t)
                {
                    for (std::size_t v = 0; v < numVerts; ++v)
                    {
                        const Vec3& d = (t < src.size() && v < src[t].size()) ? src[t][v] : Vec3{};
                        *dst++ = d.x();
                        *dst++ = d.y();
                        *dst++ = d.z();
                        *dst++ = 0.0f;
                    }
                }
            };
            writeDeltas(morphPositions);
            writeDeltas(morphNormals);
            writeDeltas(morphTangents);

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
            geoInfo.texture = resources.fallbackTexture(Resources::FallbackTextureKind::BaseColour);
        }

        // Emissive texture — use a 1x1 black dummy when material has no emissive texture
        if (binding.geometry->material().hasEmissiveTexture())
        {
            geoInfo.emissiveTexture = binding.geometry->material().emissiveTexture().handle();
        }
        else
        {
            geoInfo.emissiveTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::Emissive);
        }

        // Normal texture — use a 1x1 flat-normal dummy (128,128,255 = z-up in tangent space)
        if (binding.geometry->material().hasNormalTexture())
        {
            geoInfo.normalTexture = binding.geometry->material().normalTexture().handle();
        }
        else
        {
            geoInfo.normalTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::Normal);
        }

        // MetallicRoughness texture — use a 1x1 white dummy (1.0 metallic, 1.0 roughness)
        if (binding.geometry->material().hasMetallicRoughnessTexture())
        {
            geoInfo.metallicRoughnessTexture =
                binding.geometry->material().metallicRoughnessTexture().handle();
        }
        else
        {
            geoInfo.metallicRoughnessTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::MetallicRoughness);
        }

        // Occlusion texture — use a 1x1 white dummy (1.0 = no occlusion)
        if (binding.geometry->material().hasOcclusionTexture())
        {
            geoInfo.occlusionTexture = binding.geometry->material().occlusionTexture().handle();
        }
        else
        {
            geoInfo.occlusionTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::Occlusion);
        }

        // Transmission texture (KHR_materials_transmission). 1x1 white dummy
        // when absent so the shader's sample is harmless when the factor is 0.
        if (binding.geometry->material().hasTransmissionTexture())
        {
            geoInfo.transmissionTexture =
                binding.geometry->material().transmissionTexture().handle();
        }
        else
        {
            geoInfo.transmissionTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::Occlusion);
        }

        // Clearcoat textures (KHR_materials_clearcoat). Factor + roughness
        // fall back to the white BaseColour dummy (the shader gates them on a
        // present-flag, so the sample is harmless when absent). Normal falls
        // back to the flat-normal dummy so the TBN reconstruction still
        // produces the geometric normal.
        if (binding.geometry->material().hasClearcoatTexture())
        {
            geoInfo.clearcoatTexture = binding.geometry->material().clearcoatTexture().handle();
        }
        else
        {
            geoInfo.clearcoatTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::BaseColour);
        }
        if (binding.geometry->material().hasClearcoatRoughnessTexture())
        {
            geoInfo.clearcoatRoughnessTexture =
                binding.geometry->material().clearcoatRoughnessTexture().handle();
        }
        else
        {
            geoInfo.clearcoatRoughnessTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::BaseColour);
        }
        if (binding.geometry->material().hasClearcoatNormalTexture())
        {
            geoInfo.clearcoatNormalTexture =
                binding.geometry->material().clearcoatNormalTexture().handle();
        }
        else
        {
            geoInfo.clearcoatNormalTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::Normal);
        }

        // KHR_materials_volume thickness texture (G-channel modulates factor).
        // Fall back to white BaseColour — harmless multiplier of 1 when factor
        // is 0.
        if (binding.geometry->material().hasThicknessTexture())
        {
            geoInfo.thicknessTexture = binding.geometry->material().thicknessTexture().handle();
        }
        else
        {
            geoInfo.thicknessTexture =
                resources.fallbackTexture(Resources::FallbackTextureKind::BaseColour);
        }

        req.geometries.push_back(geoInfo);
    }

    auto descResult = resources.descriptors().createObjectDescriptors(req);
    for (std::size_t g = 0; g < bindings_.size(); ++g)
    {
        bindings_[g].descSets = descResult.descSets[g];
    }

    // Shadow descriptor sets reuse the forward skin / morph / morphSsbo
    // buffers allocated above — no duplicate uploads — plus a new per-geometry
    // ShadowUBO buffer carrying model + per-cascade lightViewProj[4] + hasSkin.
    Resources::ShadowDescriptorRequest shadowReq;
    shadowReq.geometries.reserve(bindings_.size());
    for (std::size_t g = 0; g < bindings_.size(); ++g)
    {
        auto& binding = bindings_[g];
        Resources::ShadowGeometryDescriptorInfo shadowInfo;

        ShadowUBO initialShadow{};
        initialShadow.model = Mat4::identity();
        for (Mat4& m : initialShadow.lightViewProj)
        {
            m = Mat4::identity();
        }
        auto shadowSet = resources.createMappedUniformBuffers(sizeof(ShadowUBO));
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            binding.shadowMapped[i] = shadowSet.mapped[i];
            shadowInfo.shadowUboBufs[i] = shadowSet.buffers[i];
            std::memcpy(shadowSet.mapped[i], &initialShadow, sizeof(initialShadow));
            shadowInfo.skinBufs[i] = req.geometries[g].skinBufs[i];
            shadowInfo.morphUboBufs[i] = req.geometries[g].morphUboBufs[i];
        }
        shadowInfo.morphSsbo = req.geometries[g].morphSsbo;
        shadowInfo.morphSsboSize = req.geometries[g].morphSsboSize;

        shadowReq.geometries.push_back(shadowInfo);
    }

    auto shadowDescResult = resources.descriptors().createShadowDescriptors(shadowReq);
    for (std::size_t g = 0; g < bindings_.size(); ++g)
    {
        bindings_[g].shadowDescSets = shadowDescResult.descSets[g];
    }
}

MaterialUBO Object::toMaterialUBO(const Material& mat)
{
    MaterialUBO ubo{};
    ubo.diffuseAlpha[0] = mat.diffuse().r();
    ubo.diffuseAlpha[1] = mat.diffuse().g();
    ubo.diffuseAlpha[2] = mat.diffuse().b();
    ubo.diffuseAlpha[3] = mat.alpha();
    ubo.emissiveRoughness[0] = mat.emissive().r();
    ubo.emissiveRoughness[1] = mat.emissive().g();
    ubo.emissiveRoughness[2] = mat.emissive().b();
    ubo.emissiveRoughness[3] = mat.roughness();
    ubo.materialParams[0] = mat.metallic();
    ubo.materialParams[1] = mat.normalScale();
    ubo.materialParams[2] = (mat.alphaMode() == AlphaMode::Mask) ? mat.alphaCutoff() : 0.0f;
    ubo.materialParams[3] = mat.occlusionStrength();
    ubo.textureFlags[0] = mat.hasTexture() ? 1 : 0;
    ubo.textureFlags[1] = mat.hasEmissiveTexture() ? 1 : 0;
    ubo.textureFlags[2] = mat.hasNormalTexture() ? 1 : 0;
    ubo.textureFlags[3] = mat.hasMetallicRoughnessTexture() ? 1 : 0;
    ubo.extraFlags[0] = mat.hasOcclusionTexture() ? 1 : 0;
    ubo.extraFlags[1] = mat.occlusionTexCoord();
    ubo.extraFlags[2] = mat.unlit() ? 1 : 0;
    ubo.texCoordIndices[0] = mat.baseColorTexCoord();
    ubo.texCoordIndices[1] = mat.emissiveTexCoord();
    ubo.texCoordIndices[2] = mat.normalTexCoord();
    ubo.texCoordIndices[3] = mat.metallicRoughnessTexCoord();

    auto packUv = [](float* dst, const UvTransform& t) noexcept
    {
        dst[0] = t.offsetX;
        dst[1] = t.offsetY;
        dst[2] = t.scaleX;
        dst[3] = t.scaleY;
    };
    packUv(ubo.uvBaseColor, mat.baseColorUvTransform());
    packUv(ubo.uvEmissive, mat.emissiveUvTransform());
    packUv(ubo.uvNormal, mat.normalUvTransform());
    packUv(ubo.uvMetallicRoughness, mat.metallicRoughnessUvTransform());
    packUv(ubo.uvOcclusion, mat.occlusionUvTransform());
    packUv(ubo.uvTransmission, mat.transmissionUvTransform());
    ubo.uvRotations[0] = mat.baseColorUvTransform().rotation;
    ubo.uvRotations[1] = mat.emissiveUvTransform().rotation;
    ubo.uvRotations[2] = mat.normalUvTransform().rotation;
    ubo.uvRotations[3] = mat.metallicRoughnessUvTransform().rotation;
    ubo.uvRotationsExtra[0] = mat.occlusionUvTransform().rotation;
    ubo.uvRotationsExtra[1] = mat.transmissionUvTransform().rotation;

    // KHR_materials_transmission + KHR_materials_ior.
    // .x = factor, .y = texture-present flag, .z = texCoord index, .w = ior.
    ubo.transmissionParams[0] = mat.transmissionFactor();
    ubo.transmissionParams[1] = mat.hasTransmissionTexture() ? 1.0f : 0.0f;
    ubo.transmissionParams[2] = static_cast<float>(mat.transmissionTexCoord());
    ubo.transmissionParams[3] = mat.ior();

    // KHR_materials_clearcoat. factor / roughness / normalScale, presence
    // flags, per-slot texCoord + per-slot UV transforms (offset/scale + the
    // rotation packed separately).
    ubo.clearcoatParams[0] = mat.clearcoatFactor();
    ubo.clearcoatParams[1] = mat.clearcoatRoughness();
    ubo.clearcoatParams[2] = mat.clearcoatNormalScale();
    ubo.clearcoatFlags[0] = mat.hasClearcoatTexture() ? 1.0f : 0.0f;
    ubo.clearcoatFlags[1] = mat.hasClearcoatRoughnessTexture() ? 1.0f : 0.0f;
    ubo.clearcoatFlags[2] = mat.hasClearcoatNormalTexture() ? 1.0f : 0.0f;
    ubo.clearcoatTexCoords[0] = static_cast<float>(mat.clearcoatTexCoord());
    ubo.clearcoatTexCoords[1] = static_cast<float>(mat.clearcoatRoughnessTexCoord());
    ubo.clearcoatTexCoords[2] = static_cast<float>(mat.clearcoatNormalTexCoord());
    packUv(ubo.uvClearcoat, mat.clearcoatUvTransform());
    packUv(ubo.uvClearcoatRoughness, mat.clearcoatRoughnessUvTransform());
    packUv(ubo.uvClearcoatNormal, mat.clearcoatNormalUvTransform());
    ubo.clearcoatRotations[0] = mat.clearcoatUvTransform().rotation;
    ubo.clearcoatRotations[1] = mat.clearcoatRoughnessUvTransform().rotation;
    ubo.clearcoatRotations[2] = mat.clearcoatNormalUvTransform().rotation;

    // KHR_materials_volume.
    ubo.volumeParams[0] = mat.thicknessFactor();
    ubo.volumeParams[1] = mat.hasThicknessTexture() ? 1.0f : 0.0f;
    ubo.volumeParams[2] = static_cast<float>(mat.thicknessTexCoord());
    ubo.volumeParams[3] = mat.thicknessUvTransform().rotation;
    ubo.attenuation[0] = mat.attenuationColor().r();
    ubo.attenuation[1] = mat.attenuationColor().g();
    ubo.attenuation[2] = mat.attenuationColor().b();
    // Spec defaults attenuationDistance to +inf (no absorption). Pack as a
    // very large finite number so the shader's exp(-coeff * d) collapses to
    // ~1 without inf propagating through GLSL.
    const float ad = mat.attenuationDistance();
    ubo.attenuation[3] = (ad <= 0.0f || !std::isfinite(ad)) ? 1.0e6f : ad;
    packUv(ubo.uvThickness, mat.thicknessUvTransform());
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
    ubo.proj = Mat4::perspective(cameraFovRadians, aspect, cameraNearPlane, cameraFarPlane);
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

    // Camera forward used to project draw centroids for back-to-front sort of
    // blend draws. Each mesh instance is taken as its world-translation origin
    // — fine for the scenes the engine renders today (flat decals etc.); a
    // future AABB-based centroid would be the natural upgrade.
    Vec3 forwardVec = Vec3::normalise(frame.cameraTarget - frame.cameraPosition);

    // Write shadow UBO (model + per-cascade lightViewProj[4] + hasSkin) and
    // emit a matching shadow DrawCommand alongside the forward command. The
    // renderer later buckets by pipeline so shadow draws replay inside the
    // shadow pass and forward draws replay inside the forward pass.
    ShadowUBO shadowData{};
    shadowData.model = world;
    for (std::size_t i = 0; i < frame.cascadeViewProjs.size(); ++i)
    {
        shadowData.lightViewProj[i] = frame.cascadeViewProjs[i];
    }
    shadowData.hasSkin = ubo.hasSkin;
    for (auto& binding : bindings_)
    {
        std::memcpy(binding.shadowMapped[frame.currentFrame], &shadowData, sizeof(shadowData));
    }

    // Build draw commands
    std::vector<DrawCommand> commands;
    commands.reserve(bindings_.size() * 2);
    for (const auto& binding : bindings_)
    {
        const Material& mat = binding.geometry->material();
        PipelineHandle pipe{};
        if (mat.alphaMode() == AlphaMode::Blend)
        {
            pipe = frame.pipelines.blend;
        }
        else if (mat.doubleSided())
        {
            pipe = frame.pipelines.opaqueDoubleSided;
        }
        else
        {
            pipe = frame.pipelines.opaque;
        }

        Vec3 centroid{world[0, 3], world[1, 3], world[2, 3]};
        float depth = Vec3::dotProduct(forwardVec, centroid - frame.cameraPosition);

        DrawCommand cmd;
        cmd.vertexBuffer = binding.geometry->vertexBuffer();
        cmd.indexBuffer = binding.geometry->indexBuffer();
        cmd.indexCount = binding.geometry->indexCount();
        cmd.indexType = binding.geometry->indexType();
        cmd.descriptorSet = binding.descSets[frame.currentFrame];
        cmd.pipeline = pipe;
        cmd.sortDepth = depth;
        // KHR_materials_transmission F3: defer this draw to the second forward
        // sub-pass so its fragment shader can sample the post-opaque HDR
        // target via screen-space refraction.
        cmd.transmissive = mat.transmissionFactor() > 0.0f || mat.hasTransmissionTexture();
        commands.push_back(cmd);

        if (frame.shadowPipeline != NullPipeline &&
            binding.shadowDescSets[frame.currentFrame] != NullDescriptorSet)
        {
            DrawCommand shadowCmd = cmd;
            shadowCmd.pipeline = frame.shadowPipeline;
            shadowCmd.descriptorSet = binding.shadowDescSets[frame.currentFrame];
            shadowCmd.sortDepth = 0.0f;
            commands.push_back(shadowCmd);
        }
    }
    return commands;
}

} // namespace fire_engine
