#version 450

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
} ubo;

layout(binding = 1) uniform MaterialUBO {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
    float shininess;
    float ior;
    float transparency;
    int illum;
    float roughness;
    float metallic;
    float sheen;
    float clearcoat;
    float clearcoatRoughness;
    float anisotropy;
    float anisotropyRotation;
    float alphaCutoff;
    int hasTexture;
    int hasEmissiveTexture;
    int hasNormalTexture;
    int hasMetallicRoughnessTexture;
    int hasOcclusionTexture;
} material;

layout(binding = 2) uniform sampler2D texSampler;

layout(binding = 6) uniform sampler2D emissiveMap;
layout(binding = 7) uniform sampler2D normalMap;
layout(binding = 8) uniform sampler2D metallicRoughnessMap;
layout(binding = 9) uniform sampler2D occlusionMap;
layout(binding = 10) uniform sampler2DShadow shadowMap;
layout(binding = 12) uniform samplerCube irradianceMap;

layout(binding = 11) uniform LightUBO {
    vec4 direction;
    vec4 colour;
    mat4 lightViewProj;
} light;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat3 fragTBN;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
// Keep a little fill light so materials stay readable without flattening contrast.
const float ambientStrength = 0.04;
// Shadows should stay legible, but the ambient floor must not erase separation.
const float shadowAmbientFloor = 0.15;

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

float computeShadow(vec3 worldPos)
{
    vec4 lightSpace = light.lightViewProj * vec4(worldPos, 1.0);
    vec3 proj = lightSpace.xyz / lightSpace.w;
    proj.xy = proj.xy * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.z < 0.0)
        return 1.0;
    float bias = 0.0015;
    return texture(shadowMap, vec3(proj.xy, proj.z - bias));
}

void main() {
    vec3 N;
    if (material.hasNormalTexture == 1) {
        vec3 mapNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
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
    if (material.hasTexture == 1) {
        texColor = texture(texSampler, fragTexCoord);
    }
    vec3 baseColor = material.diffuse * fragColor * texColor.rgb;

    // Alpha
    float alpha;
    if (material.hasTexture == 1) {
        alpha = (1.0 - material.transparency) * texColor.a;
    } else {
        alpha = 1.0 - material.transparency;
    }
    if (alpha < material.alphaCutoff) discard;

    // Metallic/roughness — sample from texture if available
    float roughness = material.roughness;
    float metallic = material.metallic;
    if (material.hasMetallicRoughnessTexture == 1) {
        vec4 mrSample = texture(metallicRoughnessMap, fragTexCoord);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    // Choose PBR path (glTF) vs legacy Blinn-Phong path (OBJ/MTL)
    vec3 diffuseTerm;
    vec3 specularTerm;

    if (material.specular == vec3(0.0)) {
        // --- Cook-Torrance PBR path (glTF materials) ---
        vec3 F0 = mix(vec3(0.04), baseColor, metallic);
        float a = roughness * roughness;

        float D = distributionGGX(NdotH, a);
        float G = geometrySmith(NdotV, NdotL, roughness);
        vec3  F = fresnelSchlick(VdotH, F0);

        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        specularTerm = (numerator / denominator) * lightColor * NdotL;

        // Energy-conserving diffuse: metals have no diffuse
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        diffuseTerm = kD * baseColor * (1.0 / PI) * lightColor * NdotL;
    } else {
        // --- Legacy Blinn-Phong path (OBJ/MTL materials) ---
        diffuseTerm = NdotL * baseColor;

        float spec = 0.0;
        if (NdotL > 0.0) {
            float shine = max(material.shininess, 1.0) * max(1.0 - roughness, 0.01);
            spec = pow(NdotH, shine);
        }
        specularTerm = spec * material.specular * (1.0 - roughness);
    }

    vec3 ambientBase;
    if (material.specular == vec3(0.0)) {
        vec3 F0 = mix(vec3(0.04), baseColor, metallic);
        vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 irradiance = texture(irradianceMap, N).rgb;
        ambientBase = irradiance * baseColor * kD;
    } else {
        if (material.ambient == vec3(0.0)) {
            vec3 F0 = mix(vec3(0.04), baseColor, metallic);
            vec3 diffuseAmbient = baseColor;
            vec3 specularAmbient = F0;
            ambientBase = mix(diffuseAmbient, specularAmbient, metallic);
        } else {
            ambientBase = material.ambient;
        }
        ambientBase *= ambientStrength;
    }
    vec3 ambientTerm = ambientBase;
    if (material.hasOcclusionTexture == 1) {
        float ao = texture(occlusionMap, fragTexCoord).r;
        ambientTerm *= ao;
    }

    // Emissive
    vec3 emissiveTerm = material.emissive;
    if (material.hasEmissiveTexture == 1) {
        emissiveTerm *= texture(emissiveMap, fragTexCoord).rgb;
    }

    float shadow = computeShadow(fragWorldPos);
    diffuseTerm *= shadow;
    specularTerm *= shadow;

    if (material.specular != vec3(0.0)) {
        ambientTerm *= mix(shadowAmbientFloor, 1.0, shadow);
    }

    vec3 color = ambientTerm + diffuseTerm + specularTerm + emissiveTerm;
    outColor = vec4(color, alpha);
}
