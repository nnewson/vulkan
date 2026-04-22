#version 450

layout(push_constant) uniform EnvironmentCaptureUBO {
    int faceIndex;
    int faceExtent;
} capture;

layout(binding = 0) uniform sampler2D equirectangularMap;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;

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

vec2 sampleSphericalMap(vec3 dir)
{
    float phi = atan(dir.z, dir.x);
    float theta = asin(clamp(dir.y, -1.0, 1.0));
    return vec2(0.5 + phi / (2.0 * PI), 0.5 - theta / PI);
}

void main()
{
    float extent = float(capture.faceExtent);
    vec2 texel = gl_FragCoord.xy - vec2(0.5);
    float u = (2.0 * ((texel.x + 0.5) / extent)) - 1.0;
    float v = (2.0 * ((texel.y + 0.5) / extent)) - 1.0;
    vec3 dir = directionForCubemapFace(capture.faceIndex, u, -v);
    vec2 uv = sampleSphericalMap(dir);
    uv.x = fract(uv.x);
    uv.y = clamp(uv.y, 0.0, 1.0);
    outColor = texture(equirectangularMap, uv);
}
