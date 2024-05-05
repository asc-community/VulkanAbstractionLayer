#version 460

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec3 iTangent;
layout(location = 4) in vec3 iBitangent;
layout(location = 5) in vec3 iInstancePosition;
layout(location = 6) in uint iMaterialIndex;

layout(location = 0) out vec3 vPosition;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out flat uint vMaterialIndex;
layout(location = 3) out mat3 vNormalMatrix;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    mat4 uView;
    vec3 uCameraPosition;
};

layout(set = 0, binding = 1) uniform uModelBuffer
{
    mat3 uModel;
};

void main() 
{
    vPosition = iPosition;
    gl_Position = vec4(vPosition, 1.0);
    vTexCoord = iTexCoord; 
    vMaterialIndex = iMaterialIndex;
    vNormalMatrix = uModel * mat3(iTangent, iBitangent, iNormal);
}