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
} material;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 V = normalize(ubo.cameraPos.xyz - fragWorldPos);
    vec3 H = normalize(lightDir + V);

    // Ambient
    vec3 ambientTerm = material.ambient * 0.15;

    // Diffuse (Lambertian)
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuseTerm = diff * material.diffuse * fragColor;

    // Specular (Blinn-Phong)
    float spec = 0.0;
    if (diff > 0.0) {
        float shine = max(material.shininess, 1.0);
        spec = pow(max(dot(N, H), 0.0), shine);
    }
    vec3 specularTerm = spec * material.specular;

    vec3 color = ambientTerm + diffuseTerm + specularTerm + material.emissive;
    float alpha = 1.0 - material.transparency;
    outColor = vec4(color, alpha);
}
