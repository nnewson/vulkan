#version 450

layout(location = 0) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;
const uint SAMPLE_COUNT = 128u;

float radicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint index, uint sampleCount)
{
    return vec2(float(index) / float(sampleCount), radicalInverseVdC(index));
}

vec3 importanceSampleGGX(vec2 xi, vec3 normal, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));

    vec3 halfVector = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(tangent * halfVector.x + bitangent * halfVector.y +
                     normal * halfVector.z);
}

float geometrySchlickGGXForIbl(float nDotV, float roughness)
{
    float a = roughness;
    float k = (a * a) * 0.5;
    return nDotV / (nDotV * (1.0 - k) + k);
}

float geometrySmithForIbl(float nDotV, float nDotL, float roughness)
{
    return geometrySchlickGGXForIbl(nDotV, roughness) *
           geometrySchlickGGXForIbl(nDotL, roughness);
}

void main()
{
    float nDotV = fragUv.x;
    float roughness = fragUv.y;

    vec3 view = vec3(sqrt(max(0.0, 1.0 - nDotV * nDotV)), 0.0, nDotV);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    float scale = 0.0;
    float bias = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 halfVector = importanceSampleGGX(xi, normal, roughness);
        vec3 light = normalize(halfVector * (2.0 * dot(view, halfVector)) - view);

        float nDotL = max(light.z, 0.0);
        float nDotH = max(halfVector.z, 0.0);
        float vDotH = max(dot(view, halfVector), 0.0);

        if (nDotL > 0.0)
        {
            float visibility = geometrySmithForIbl(nDotV, nDotL, roughness) * vDotH /
                               max(nDotH * nDotV, 0.0001);
            float fresnel = pow(1.0 - vDotH, 5.0);
            scale += (1.0 - fresnel) * visibility;
            bias += fresnel * visibility;
        }
    }

    scale /= float(SAMPLE_COUNT);
    bias /= float(SAMPLE_COUNT);
    outColor = vec4(scale, bias, 0.0, 1.0);
}
