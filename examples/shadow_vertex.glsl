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

layout(set = 0, binding = 1) uniform uModelBuffer
{
    mat3 uModel;
};

layout(set = 0, binding = 2) uniform uLightBuffer
{
    mat4 uLightProjection;
    vec4 uLightColorPadding;
    vec3 uLightDirection;
};

void main()
{
    vec3 worldSpacePosition = (uModel * iPosition) + iInstancePosition;
    gl_Position = uLightProjection * vec4(worldSpacePosition, 1.0);
}