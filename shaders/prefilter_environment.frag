#version 450

layout(push_constant) uniform EnvironmentPrefilterPushConstants {
    int faceIndex;
    int faceExtent;
    float roughness;
    int sourceFaceExtent;
    float sourceMaxMip;
    float _pad0;
    float _pad1;
    float _pad2;
} capture;

layout(binding = 0) uniform samplerCube environmentMap;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;
const uint SAMPLE_COUNT = 256u;

vec3 directionForCubemapFace(int face, float u, float v)
{
    switch (face)
    {
    case 0:
        return normalize(vec3(1.0, v, -u));
    case 1:
        return normalize(vec3(-1.0, v, u));
    case 2:
        return normalize(vec3(u, 1.0, -v));
    case 3:
        return normalize(vec3(u, -1.0, v));
    case 4:
        return normalize(vec3(u, v, 1.0));
    default:
        return normalize(vec3(-u, v, -1.0));
    }
}

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

void main()
{
    float extent = float(capture.faceExtent);
    vec2 texel = gl_FragCoord.xy - vec2(0.5);
    float u = (2.0 * ((texel.x + 0.5) / extent)) - 1.0;
    float v = (2.0 * ((texel.y + 0.5) / extent)) - 1.0;

    vec3 reflection = directionForCubemapFace(capture.faceIndex, u, -v);
    vec3 normal = reflection;
    vec3 view = reflection;
    vec3 prefiltered = vec3(0.0);
    float totalWeight = 0.0;

    // Filament/Karis mip-weighted importance sampling: pick a blurrier source
    // mip when the PDF is low so under-sampled rough lobes don't shimmer.
    float a = capture.roughness * capture.roughness;
    float srcExtent = float(capture.sourceFaceExtent);
    float saTexel = 4.0 * PI / (6.0 * srcExtent * srcExtent);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 halfVector = importanceSampleGGX(xi, normal, capture.roughness);
        vec3 light = normalize(halfVector * (2.0 * dot(view, halfVector)) - view);

        float nDotL = max(dot(normal, light), 0.0);
        if (nDotL > 0.0)
        {
            // PDF for importance-sampled GGX (V == N in prefilter convention).
            float nDotH = max(dot(normal, halfVector), 0.0);
            float vDotH = max(dot(view, halfVector), 0.0);
            float D = (a * a) / (PI * pow(nDotH * nDotH * (a * a - 1.0) + 1.0, 2.0));
            float pdf = (D * nDotH / (4.0 * vDotH)) + 0.0001;
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf);
            float mipLevel = capture.roughness == 0.0
                                 ? 0.0
                                 : 0.5 * log2(saSample / saTexel);
            mipLevel = clamp(mipLevel, 0.0, capture.sourceMaxMip);

            prefiltered += textureLod(environmentMap, light, mipLevel).rgb * nDotL;
            totalWeight += nDotL;
        }
    }

    if (totalWeight > 0.0)
    {
        prefiltered /= totalWeight;
    }

    outColor = vec4(prefiltered, 1.0);
}
