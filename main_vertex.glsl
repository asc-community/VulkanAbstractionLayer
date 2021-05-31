#version 460

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec3 vNormal;

layout(set = 0, binding = 1) uniform uCameraBuffer
{
    mat4 uViewProjection;
    mat3 uModel;
};

void main() 
{
    vec3 worldSpacePosition = uModel * iPosition;
    gl_Position = uViewProjection * vec4(worldSpacePosition, 1.0);
    vTexCoord = iTexCoord; 
    vNormal = mat3(uModel) * iNormal;
}