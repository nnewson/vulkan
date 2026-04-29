#version 450

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
} ubo;

layout(binding = 1) uniform MaterialUBO {
    vec4 diffuseAlpha;
    vec4 emissiveRoughness;
    vec4 materialParams;
    ivec4 textureFlags;
    // .x = occlusion-texture present, .y = occlusion's UV-set index.
    ivec4 extraFlags;
    // x=baseColor, y=emissive, z=normal, w=metallicRoughness UV-set index.
    ivec4 texCoordIndices;
    // KHR_texture_transform per-slot offset.xy + scale.xy. Identity by default.
    vec4 uvBaseColor;
    vec4 uvEmissive;
    vec4 uvNormal;
    vec4 uvMetallicRoughness;
    vec4 uvOcclusion;
    vec4 uvTransmission;
    // Rotations (radians, CCW) packed: x=base, y=emissive, z=normal, w=mr.
    vec4 uvRotations;
    // .x = occlusion rotation, .y = transmission rotation; rest reserved.
    vec4 uvRotationsExtra;
    // KHR_materials_transmission. .x = transmissionFactor, .y = texture-present
    // flag, .z = transmission texCoord index. .w reserved (IOR / thickness).
    vec4 transmissionParams;
} material;

layout(binding = 2) uniform sampler2D texSampler;

layout(binding = 6) uniform sampler2D emissiveMap;
layout(binding = 7) uniform sampler2D normalMap;
layout(binding = 8) uniform sampler2D metallicRoughnessMap;
layout(binding = 9) uniform sampler2D occlusionMap;
layout(binding = 16) uniform sampler2D transmissionMap;
layout(binding = 10) uniform sampler2DArrayShadow shadowMap;
layout(binding = 12) uniform samplerCube irradianceMap;
layout(binding = 13) uniform samplerCube prefilteredMap;
layout(binding = 14) uniform sampler2D brdfLut;
// PCSS blocker search reads raw depths from the same shadow image via a
// non-comparison sampler. Binding 10 is for the hardware-PCF compare path.
layout(binding = 15) uniform sampler2DArray shadowMapDepth;

struct LightData {
    // .xyz = world position (point/spot), .w = type (0=dir, 1=point, 2=spot)
    vec4 position;
    // .xyz = world forward, .w = range (point/spot; 0 = infinite)
    vec4 direction;
    // .rgb = colour, .a = intensity
    vec4 colour;
    // .x = cos(innerCone), .y = cos(outerCone)
    vec4 cone;
};

const int MAX_LIGHTS = 8;

layout(binding = 11) uniform LightUBO {
    mat4 cascadeViewProj[4];
    vec4 cascadeSplits;
    vec4 iblParams;
    vec4 shadowParams;
    vec4 environmentParams;
    int  lightCount;
    int  _pad0;
    int  _pad1;
    int  _pad2;
    LightData lights[MAX_LIGHTS];
} light;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat3 fragTBN;
layout(location = 7) in float fragViewDepth;
layout(location = 8) in vec2 fragTexCoord1;

layout(location = 0) out vec4 outColor;

vec2 pickUv(int index)
{
    return index == 0 ? fragTexCoord : fragTexCoord1;
}

// KHR_texture_transform: scale → rotate → translate (CCW around origin).
// offsetScale.xy = offset, offsetScale.zw = scale.
vec2 applyUvTransform(vec2 uv, vec4 offsetScale, float rotation)
{
    vec2 scaled = uv * offsetScale.zw;
    float c = cos(rotation);
    float s = sin(rotation);
    vec2 rotated = vec2(c * scaled.x - s * scaled.y,
                        s * scaled.x + c * scaled.y);
    return rotated + offsetScale.xy;
}

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz normal distribution
float distributionGGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Schlick-GGX geometry term (single direction)
float geometrySchlickGGX(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Smith's method combining view and light geometry terms
float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return geometrySchlickGGX(NdotV, k) * geometrySchlickGGX(NdotL, k);
}

// Schlick Fresnel approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 16-tap Poisson disk for PCSS blocker search and variable-radius PCF.
const vec2 poissonDisk[16] = vec2[16](
    vec2(-0.94201624, -0.39906216), vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870), vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845), vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554), vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507), vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367), vec2( 0.14383161, -0.14100790)
);

// Per-pixel rotation hash so neighbouring fragments use different rotations
// of the same Poisson kernel. Stops the kernel pattern from showing as moiré.
mat2 poissonRotation(vec3 worldPos)
{
    float h = fract(sin(dot(worldPos.xy + worldPos.zx, vec2(12.9898, 78.233))) * 43758.5453);
    float c = cos(h * 6.283185);
    float s = sin(h * 6.283185);
    return mat2(c, -s, s, c);
}

float pcfSample(vec3 worldPos, vec3 normal, vec3 lightDir, int cascade)
{
    vec4 lightSpace = light.cascadeViewProj[cascade] * vec4(worldPos, 1.0);
    vec3 proj = lightSpace.xyz / lightSpace.w;
    proj.xy = proj.xy * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.z < 0.0)
        return 1.0;

    float minBias = light.shadowParams.x;
    float slopeBias = light.shadowParams.y;
    float lightSize = light.shadowParams.w;
    float baseBias = max(minBias, slopeBias * (1.0 - max(dot(normal, lightDir), 0.0)));
    // Far cascades cover proportionally more world per texel; bias must scale.
    float bias = baseBias * exp2(float(cascade));
    float receiverDepth = proj.z - bias;

    mat2 rot = poissonRotation(worldPos);

    // 1. Blocker search — average depth of any occluders within search radius.
    //    Reads raw depths via the non-comparison sampler at binding 15.
    float searchRadius = lightSize;
    float blockerSum = 0.0;
    int blockerCount = 0;
    for (int i = 0; i < 16; ++i) {
        vec2 off = rot * poissonDisk[i] * searchRadius;
        float d = texture(shadowMapDepth, vec3(proj.xy + off, float(cascade))).r;
        if (d < receiverDepth) {
            blockerSum += d;
            blockerCount++;
        }
    }
    if (blockerCount == 0) {
        // No occluders → fully lit, skip the PCF cost entirely.
        return 1.0;
    }
    float avgBlocker = blockerSum / float(blockerCount);

    // 2. Penumbra size from blocker / receiver depth ratio (PCSS classical).
    //    Larger gap → softer shadow.
    float penumbra = (receiverDepth - avgBlocker) / max(avgBlocker, 1e-4) * lightSize;
    // Floor at one shadow texel — keeps the kernel from collapsing to a single
    // sample at razor-thin contact, which would re-introduce aliasing.
    float texelSize = 1.0 / float(textureSize(shadowMap, 0).x);
    float filterRadius = max(penumbra, texelSize);

    // 3. Variable-radius PCF — same Poisson kernel, hardware compare sampler.
    float vis = 0.0;
    for (int i = 0; i < 16; ++i) {
        vec2 off = rot * poissonDisk[i] * filterRadius;
        vis += texture(shadowMap, vec4(proj.xy + off, float(cascade), receiverDepth));
    }
    return vis / 16.0;
}

int selectCascade(float viewDepth)
{
    int cascade = 3;
    for (int i = 0; i < 4; ++i)
    {
        if (viewDepth < light.cascadeSplits[i])
        {
            cascade = i;
            break;
        }
    }
    return cascade;
}

// Blend factor in the last 10% of a cascade's view-space range. 0.0 = pure
// current cascade; 1.0 = pure next cascade. Always 0.0 for the last cascade.
float cascadeBlendFactor(int cascade, float viewDepth)
{
    if (cascade >= 3)
        return 0.0;
    float cascadeStart = cascade == 0 ? 0.0 : light.cascadeSplits[cascade - 1];
    float cascadeEnd = light.cascadeSplits[cascade];
    float blendBand = (cascadeEnd - cascadeStart) * 0.1;
    float blendStart = cascadeEnd - blendBand;
    return clamp((viewDepth - blendStart) / blendBand, 0.0, 1.0);
}

float computeShadow(vec3 worldPos, vec3 normal, vec3 lightDir, int cascade, float viewDepth)
{
    float current = pcfSample(worldPos, normal, lightDir, cascade);
    float t = cascadeBlendFactor(cascade, viewDepth);
    if (t <= 0.0)
        return current;
    float next = pcfSample(worldPos, normal, lightDir, cascade + 1);
    return mix(current, next, t);
}

void main() {
    vec3 N;
    if (material.textureFlags.z == 1) {
        vec2 uvNormal = applyUvTransform(pickUv(material.texCoordIndices.z),
                                         material.uvNormal, material.uvRotations.z);
        vec3 mapNormal = texture(normalMap, uvNormal).rgb * 2.0 - 1.0;
        mapNormal.xy *= material.materialParams.y;
        N = normalize(fragTBN * mapNormal);
    } else {
        N = normalize(fragNormal);
    }

    vec3 V = normalize(ubo.cameraPos.xyz - fragWorldPos);
    float NdotV = max(dot(N, V), 0.001);

    // Sample base colour texture once
    vec4 texColor = vec4(1.0);
    if (material.textureFlags.x == 1) {
        vec2 uvBase = applyUvTransform(pickUv(material.texCoordIndices.x),
                                       material.uvBaseColor, material.uvRotations.x);
        texColor = texture(texSampler, uvBase);
    }
    vec3 baseColor = material.diffuseAlpha.rgb * fragColor * texColor.rgb;

    // Alpha
    float alpha = material.diffuseAlpha.a;
    if (material.textureFlags.x == 1) {
        alpha *= texColor.a;
    }
    if (alpha < material.materialParams.z) discard;

    // KHR_materials_unlit. Skip BRDF/IBL/shadow entirely; output the textured
    // base colour directly. Post-process tonemap still runs on the HDR target.
    if (material.extraFlags.z == 1) {
        outColor = vec4(baseColor, alpha);
        return;
    }

    // Metallic/roughness — sample from texture if available
    float roughness = material.emissiveRoughness.a;
    float metallic = material.materialParams.x;
    if (material.textureFlags.w == 1) {
        vec2 uvMr = applyUvTransform(pickUv(material.texCoordIndices.w),
                                     material.uvMetallicRoughness, material.uvRotations.w);
        vec4 mrSample = texture(metallicRoughnessMap, uvMr);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    float a = roughness * roughness;

    // Direct lighting loop — accumulate contributions from every light in
    // LightUBO::lights[]. Only the first directional (i==0, type==0) gets CSM
    // shadow; everything else is unshadowed.
    vec3 directDiffuse = vec3(0.0);
    vec3 directSpecular = vec3(0.0);
    for (int i = 0; i < light.lightCount && i < MAX_LIGHTS; ++i) {
        LightData L = light.lights[i];
        int type = int(L.position.w);

        // KHR_lights_punctual stores forward (light-to-target). Negate to get
        // the surface-to-light vector the BRDF wants.
        vec3 lightVec;
        float attenuation = 1.0;
        if (type == 0) {
            lightVec = normalize(-L.direction.xyz);
        } else {
            // Point/spot share the inverse-square + range-windowed falloff
            // from KHR_lights_punctual:
            //   windowing = clamp(1 - (d/range)^4, 0, 1)
            //   attenuation = windowing^2 / max(d^2, 0.01)
            // Range == 0 means "no range cutoff" — windowing collapses to 1.
            vec3 toLight = L.position.xyz - fragWorldPos;
            float dist = length(toLight);
            lightVec = toLight / max(dist, 1e-4);
            float range = L.direction.w;
            float windowing = (range > 0.0)
                ? clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0)
                : 1.0;
            attenuation = (windowing * windowing) / max(dist * dist, 0.01);

            if (type == 2) {
                // KHR_lights_punctual spot: cosTheta is the angle between
                // the spot's forward (light-to-target) and the light-to-frag
                // vector. lightVec points surface-to-light, so the
                // light-to-frag vector is -lightVec.
                float cosTheta = -dot(normalize(L.direction.xyz), lightVec);
                float spotFactor = clamp((cosTheta - L.cone.y)
                                         / max(L.cone.x - L.cone.y, 1e-4),
                                         0.0, 1.0);
                attenuation *= spotFactor * spotFactor;
            }
        }

        vec3 lightColor = L.colour.rgb * L.colour.a * attenuation;
        vec3 H = normalize(lightVec + V);
        float NdotL = max(dot(N, lightVec), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        float D = distributionGGX(NdotH, a);
        float G = geometrySmith(NdotV, NdotL, roughness);
        vec3 F = fresnelSchlick(VdotH, F0);

        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specularContrib = (numerator / denominator) * lightColor * NdotL;
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 diffuseContrib = kD * baseColor * (1.0 / PI) * lightColor * NdotL;

        if (i == 0 && type == 0) {
            int cascade = selectCascade(fragViewDepth);
            float shadow = computeShadow(fragWorldPos, N, lightVec, cascade, fragViewDepth);
            diffuseContrib *= shadow;
            specularContrib *= shadow;
        }

        directDiffuse += diffuseContrib;
        directSpecular += specularContrib;
    }

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 R = reflect(-V, N);
    float maxReflectionLod = light.iblParams.x;
    vec3 prefilteredColor = textureLod(prefilteredMap, R, roughness * maxReflectionLod).rgb;
    vec2 envBrdf = texture(brdfLut, vec2(NdotV, roughness)).rg;

    // Fdez-Aguera multi-scatter compensation. Recovers the energy the split-sum
    // single-scatter lobe loses on rough conductors.
    vec3 FssEss = F0 * envBrdf.x + envBrdf.y;
    float Ess = envBrdf.x + envBrdf.y;
    float Ems = 1.0 - Ess;
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 Fms = FssEss * Favg / (1.0 - Ems * Favg);
    vec3 multiScatter = Fms * Ems;

    vec3 iblKD = baseColor * (1.0 - FssEss - multiScatter) * (1.0 - metallic);
    vec3 diffuseIbl = irradiance * iblKD * light.iblParams.y;
    vec3 specularIbl = prefilteredColor * (FssEss + multiScatter) * light.iblParams.z;

    float ao = 1.0;
    if (material.extraFlags.x == 1) {
        // glTF spec: occluded = lerp(colour, colour * sampled, strength).
        // Equivalent to ao = mix(1.0, sampled, strength) when applied as a
        // multiplier downstream.
        vec2 uvOcc = applyUvTransform(pickUv(material.extraFlags.y),
                                      material.uvOcclusion, material.uvRotationsExtra.x);
        float sampled = texture(occlusionMap, uvOcc).r;
        ao = mix(1.0, sampled, material.materialParams.w);
    }

    vec3 diffuseAmbientTerm = diffuseIbl * ao;
    float specularAo = mix(1.0, ao, 0.25);
    vec3 specularAmbientTerm = specularIbl * specularAo;
    vec3 ambientTerm = diffuseAmbientTerm + specularAmbientTerm;

    // Emissive
    vec3 emissiveTerm = material.emissiveRoughness.rgb;
    if (material.textureFlags.y == 1) {
        vec2 uvEm = applyUvTransform(pickUv(material.texCoordIndices.y),
                                     material.uvEmissive, material.uvRotations.y);
        emissiveTerm *= texture(emissiveMap, uvEm).rgb;
    }

    // KHR_materials_transmission (F2 — IBL-faked refraction). Per glTF spec,
    // the diffuse lobe is *attenuated* by (1 - transmission) and a separate
    // transmission lobe is added on top — specular is left intact. For glass
    // against the environment this is sufficient; proper scene-behind-glass
    // refraction (F3) would copy the HDR target into a sceneColor mip chain.
    float transmission = material.transmissionParams.x;
    if (material.transmissionParams.y > 0.5) {
        vec2 uvTrans = applyUvTransform(pickUv(int(material.transmissionParams.z)),
                                        material.uvTransmission, material.uvRotationsExtra.y);
        transmission *= texture(transmissionMap, uvTrans).r;
    }

    vec3 transmittedLight = vec3(0.0);
    if (transmission > 0.0) {
        // ior fixed at 1.5 for first cut. Refracted direction sampled at the
        // same LOD selection used by specular so frosted-glass blur falls
        // out for free.
        const float ior = 1.5;
        vec3 refractDir = refract(-V, N, 1.0 / ior);
        if (dot(refractDir, refractDir) < 1e-6) {
            // Total internal reflection at grazing angles → fall back to R.
            refractDir = R;
        }
        vec3 transmissionSample =
            textureLod(prefilteredMap, refractDir, roughness * maxReflectionLod).rgb;
        // Match the specular IBL brightness budget so dark base-colour texels
        // (e.g. newspaper text) retain visible contrast against bright env.
        transmittedLight = transmission * baseColor * transmissionSample * light.iblParams.z;
    }

    // Diffuse lobes are scaled — NOT replaced — by (1 - transmission). Specular
    // and emissive paths are unchanged.
    directDiffuse      *= (1.0 - transmission);
    diffuseAmbientTerm *= (1.0 - transmission);

    vec3 color = diffuseAmbientTerm + specularAmbientTerm
               + directDiffuse + directSpecular
               + transmittedLight + emissiveTerm;

    if (light.environmentParams.w > 0.5) {
        int cascade = selectCascade(fragViewDepth);
        vec3 cascadeTints[4] = vec3[4](
            vec3(1.0, 0.4, 0.4),  // cascade 0 — red, closest
            vec3(0.4, 1.0, 0.4),  // cascade 1 — green
            vec3(0.4, 0.4, 1.0),  // cascade 2 — blue
            vec3(1.0, 1.0, 0.4)   // cascade 3 — yellow, furthest
        );
        vec3 currentTint = cascadeTints[cascade];
        float t = cascadeBlendFactor(cascade, fragViewDepth);
        vec3 nextTint = cascadeTints[min(cascade + 1, 3)];
        color *= mix(currentTint, nextTint, t);
    }

    outColor = vec4(color, alpha);
}
