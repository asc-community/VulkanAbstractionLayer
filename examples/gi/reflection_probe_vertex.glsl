#version 460

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec3 iTangent;
layout(location = 4) in vec3 iBitangent;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec3 vPosition;
layout(location = 1) out vec3 vNormal;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(push_constant) uniform uProbeContant
{
    vec3 uProbePosition;
    float uProbeSize;
    uint uProbeCubemapIndex;
};


void main() 
{
    vPosition = uProbeSize * iPosition + uProbePosition;
    gl_Position = uViewProjection * vec4(vPosition, 1.0);
    vNormal = iNormal;
}