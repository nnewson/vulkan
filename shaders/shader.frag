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
} material;

layout(binding = 2) uniform sampler2D texSampler;

layout(binding = 6) uniform sampler2D emissiveMap;
layout(binding = 7) uniform sampler2D normalMap;
layout(binding = 8) uniform sampler2D metallicRoughnessMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in mat3 fragTBN;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N;
    if (material.hasNormalTexture == 1) {
        vec3 mapNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        N = normalize(fragTBN * mapNormal);
    } else {
        N = normalize(fragNormal);
    }
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 V = normalize(ubo.cameraPos.xyz - fragWorldPos);
    vec3 H = normalize(lightDir + V);

    // Ambient
    vec3 ambientTerm = material.ambient * 0.15;

    // Diffuse (Lambertian)
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuseTerm;
    float alpha;

    if (material.hasTexture == 1) {
        vec4 texColor = texture(texSampler, fragTexCoord);
        diffuseTerm = diff * material.diffuse * fragColor * texColor.rgb;
        alpha = (1.0 - material.transparency) * texColor.a;
    } else {
        diffuseTerm = diff * material.diffuse * fragColor;
        alpha = 1.0 - material.transparency;
    }

    // Metallic/roughness — sample from texture if available
    float roughness = material.roughness;
    float metallic = material.metallic;
    if (material.hasMetallicRoughnessTexture == 1) {
        vec4 mrSample = texture(metallicRoughnessMap, fragTexCoord);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }

    // Specular (Blinn-Phong with roughness influence)
    float spec = 0.0;
    if (diff > 0.0) {
        float shine = max(material.shininess, 1.0) * max(1.0 - roughness, 0.01);
        spec = pow(max(dot(N, H), 0.0), shine);
    }
    vec3 specularTerm = spec * material.specular * (1.0 - roughness);

    if (alpha < material.alphaCutoff) discard;

    vec3 emissiveTerm = material.emissive;
    if (material.hasEmissiveTexture == 1) {
        emissiveTerm *= texture(emissiveMap, fragTexCoord).rgb;
    }

    vec3 color = ambientTerm + diffuseTerm + specularTerm + emissiveTerm;
    outColor = vec4(color, alpha);
}
