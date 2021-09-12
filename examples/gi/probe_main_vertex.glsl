#version 460

#extension GL_EXT_multiview : enable

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
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out mat3 vNormalMatrix;

layout(push_constant) uniform uPushConstant
{
     vec3 uCameraPosition;
     uint uMaterialIndex;
     vec3 uProbeGridOffset;
     vec3 uProbeGridDensity;
     vec3 uProbeGridSize;
};

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition_Unused;
};

layout(set = 0, binding = 1) uniform uProbeViewsBuffer
{
    mat4 uProbeMatrices[6];
};

layout(set = 0, binding = 2) uniform uModelBuffer
{
    mat3 uModel;
};

void main() 
{
    vPosition = uModel * iPosition;
    gl_Position = uProbeMatrices[gl_ViewIndex] * vec4(vPosition, 1.0);
    vTexCoord = iTexCoord;
    vNormalMatrix = uModel * mat3(iTangent, iBitangent, iNormal);
}