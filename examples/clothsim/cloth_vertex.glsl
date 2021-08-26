#version 460

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

layout(set = 0, binding = 1) uniform sampler2D uPositionImage;

layout(push_constant) uniform uColorPushConstants
{
    vec4 uColor_QuadsPerRow;
};

const ivec2 QuadIndexOffset[] = ivec2[](
    ivec2(0, 0),
    ivec2(1, 0),
    ivec2(1, 1),
    ivec2(0, 0),
    ivec2(1, 1),
    ivec2(0, 1),

    ivec2(0, 0),
    ivec2(1, 1),
    ivec2(1, 0),
    ivec2(0, 0),
    ivec2(0, 1),
    ivec2(1, 1)
);

void main() 
{
    uint quadIndex = gl_VertexIndex / 12;
    uint quadVertex = gl_VertexIndex % 12;
    ivec2 offset = QuadIndexOffset[quadVertex];
    uint x = quadIndex / uint(uColor_QuadsPerRow.a);
    uint y = quadIndex % uint(uColor_QuadsPerRow.a);

    vec3 position = texelFetch(uPositionImage, ivec2(x, y) + offset, 0).rgb;

    vec3 neighborPositions[4];
    neighborPositions[0] = texelFetch(uPositionImage, ivec2(x - 1, y) + offset, 0).rgb;
    neighborPositions[1] = texelFetch(uPositionImage, ivec2(x + 1, y) + offset, 0).rgb;
    neighborPositions[2] = texelFetch(uPositionImage, ivec2(x, y - 1) + offset, 0).rgb;
    neighborPositions[3] = texelFetch(uPositionImage, ivec2(x, y + 1) + offset, 0).rgb;
    vNormal = cross(
        normalize(neighborPositions[1] - neighborPositions[0]),
        normalize(neighborPositions[3] - neighborPositions[2])
    );

    vPosition = position;
    gl_Position = uViewProjection * vec4(vPosition, 1.0);
}