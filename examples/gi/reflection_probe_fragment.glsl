#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(set = 0, binding = 1) uniform textureCube uProbeArray[256];

layout(push_constant) uniform uProbeContant
{
    vec4 uProbePosition_ProbeSize;
    uint uProbeCubemapIndex;
};

layout(set = 0, binding = 2) uniform sampler uTextureSampler;

void main()
{
    vec3 probeColor = texture(samplerCube(uProbeArray[uProbeCubemapIndex], uTextureSampler), vNormal).rgb;
    oColor = vec4(probeColor, 1.0);
}