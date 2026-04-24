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
    ivec4 extraFlags;
} material;

layout(binding = 2) uniform sampler2D texSampler;

layout(binding = 6) uniform sampler2D emissiveMap;
layout(binding = 7) uniform sampler2D normalMap;
layout(binding = 8) uniform sampler2D metallicRoughnessMap;
layout(binding = 9) uniform sampler2D occlusionMap;
layout(binding = 10) uniform sampler2DArrayShadow shadowMap;
layout(binding = 12) uniform samplerCube irradianceMap;
layout(binding = 13) uniform samplerCube prefilteredMap;
layout(binding = 14) uniform sampler2D brdfLut;

layout(binding = 11) uniform LightUBO {
    vec4 direction;
    vec4 colour;
    mat4 cascadeViewProj[4];
    vec4 cascadeSplits;
    vec4 iblParams;
    vec4 shadowParams;
    vec4 environmentParams;
} light;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat3 fragTBN;

layout(location = 0) out vec4 outColor;

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
                    pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float computeShadow(vec3 worldPos, vec3 normal, vec3 lightDir)
{
    // Cascade 0 sampled explicitly — Stage 4d will select per fragment.
    vec4 lightSpace = light.cascadeViewProj[0] * vec4(worldPos, 1.0);
    vec3 proj = lightSpace.xyz / lightSpace.w;
    proj.xy = proj.xy * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.z < 0.0)
        return 1.0;

    float minBias = light.shadowParams.x;
    float slopeBias = light.shadowParams.y;
    float filterRadius = light.shadowParams.z;
    float bias = max(minBias, slopeBias * (1.0 - max(dot(normal, lightDir), 0.0)));

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0).xy);
    float visibility = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize * filterRadius;
            // sampler2DArrayShadow: (u, v, layer, depthRef). Layer is fixed at
            // 0 until Stage 4d adds per-fragment cascade selection.
            visibility += texture(shadowMap, vec4(proj.xy + offset, 0.0, proj.z - bias));
        }
    }
    return visibility / 9.0;
}

void main() {
    vec3 N;
    if (material.textureFlags.z == 1) {
        vec3 mapNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        mapNormal.xy *= material.materialParams.y;
        N = normalize(fragTBN * mapNormal);
    } else {
        N = normalize(fragNormal);
    }

    vec3 lightDir = normalize(light.direction.xyz);
    vec3 lightColor = light.colour.rgb * light.colour.a;
    vec3 V = normalize(ubo.cameraPos.xyz - fragWorldPos);
    vec3 H = normalize(lightDir + V);

    float NdotL = max(dot(N, lightDir), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Sample base colour texture once
    vec4 texColor = vec4(1.0);
    if (material.textureFlags.x == 1) {
        texColor = texture(texSampler, fragTexCoord);
    }
    vec3 baseColor = material.diffuseAlpha.rgb * fragColor * texColor.rgb;

    // Alpha
    float alpha = material.diffuseAlpha.a;
    if (material.textureFlags.x == 1) {
        alpha *= texColor.a;
    }
    if (alpha < material.materialParams.z) discard;

    // Metallic/roughness — sample from texture if available
    float roughness = material.emissiveRoughness.a;
    float metallic = material.materialParams.x;
    if (material.textureFlags.w == 1) {
        vec4 mrSample = texture(metallicRoughnessMap, fragTexCoord);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    float a = roughness * roughness;
    float D = distributionGGX(NdotH, a);
    float G = geometrySmith(NdotV, NdotL, roughness);
    vec3 F = fresnelSchlick(VdotH, F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specularTerm = (numerator / denominator) * lightColor * NdotL;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuseTerm = kD * baseColor * (1.0 / PI) * lightColor * NdotL;

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
        ao = texture(occlusionMap, fragTexCoord).r;
    }

    vec3 diffuseAmbientTerm = diffuseIbl * ao;
    float specularAo = mix(1.0, ao, 0.25);
    vec3 specularAmbientTerm = specularIbl * specularAo;
    vec3 ambientTerm = diffuseAmbientTerm + specularAmbientTerm;

    // Emissive
    vec3 emissiveTerm = material.emissiveRoughness.rgb;
    if (material.textureFlags.y == 1) {
        emissiveTerm *= texture(emissiveMap, fragTexCoord).rgb;
    }

    float shadow = computeShadow(fragWorldPos, N, lightDir);
    diffuseTerm *= shadow;
    specularTerm *= shadow;

    vec3 color = ambientTerm + diffuseTerm + specularTerm + emissiveTerm;
    outColor = vec4(color, alpha);
}
