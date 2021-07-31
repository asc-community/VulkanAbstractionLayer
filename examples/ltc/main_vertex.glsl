#version 460

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec3 vPosition;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out vec3 vNormal;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(set = 0, binding = 1) uniform uModelBuffer
{
    mat3 uModel;
};

void main() 
{
    vPosition = uModel * iPosition;
    gl_Position = uViewProjection * vec4(vPosition, 1.0);
    vTexCoord = iTexCoord; 
    vNormal = uModel * iNormal;
}