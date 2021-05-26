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
};

void main() 
{
    mat4 M = mat4(
        vec4(-0.707107, -0.408248, 0.57735, 0),
        vec4(0.707107, - 0.408248, 0.57735, 0),
        vec4(0, 0.816497, 0.57735, 0),
        vec4(-0, - 0, - 86.6025, 1)
    );

    gl_Position = uViewProjection * vec4(iPosition, 1.0);
    vTexCoord = iTexCoord; 
    vNormal = iNormal;
}