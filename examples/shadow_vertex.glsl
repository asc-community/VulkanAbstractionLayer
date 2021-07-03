#version 460

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 unused_1;
layout(location = 2) in vec3 unused_2;
layout(location = 3) in vec3 iInstancePosition;
layout(location = 4) in uint unused_3;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
};

layout(set = 0, binding = 1) uniform uModelBuffer
{
    mat3 uModel;
};

void main()
{
    vec3 worldSpacePosition = (uModel * iPosition) + iInstancePosition;
    gl_Position = uViewProjection * vec4(worldSpacePosition, 1.0);
}