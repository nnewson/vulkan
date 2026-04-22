#version 450

layout(push_constant) uniform EnvironmentCaptureUBO {
    int faceIndex;
    int faceExtent;
} capture;

layout(binding = 0) uniform samplerCube environmentMap;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;
const float SAMPLE_DELTA = 0.2;

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

void main()
{
    float extent = float(capture.faceExtent);
    vec2 texel = gl_FragCoord.xy - vec2(0.5);
    float u = (2.0 * ((texel.x + 0.5) / extent)) - 1.0;
    float v = (2.0 * ((texel.y + 0.5) / extent)) - 1.0;

    vec3 normal = directionForCubemapFace(capture.faceIndex, u, -v);
    vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    vec3 irradiance = vec3(0.0);
    float sampleCount = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += SAMPLE_DELTA)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += SAMPLE_DELTA)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = right * tangentSample.x + up * tangentSample.y +
                             normal * tangentSample.z;
            float weight = cos(theta) * sin(theta);
            irradiance += texture(environmentMap, normalize(sampleVec)).rgb * weight;
            sampleCount += 1.0;
        }
    }

    if (sampleCount > 0.0)
    {
        irradiance *= PI / sampleCount;
    }

    outColor = vec4(irradiance, 1.0);
}
