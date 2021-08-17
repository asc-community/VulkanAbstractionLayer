#version 460

out gl_PerVertex
{
    vec4 gl_Position;
};

const vec3 SkyboxVertices[] = vec3[](
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),

    vec3(-1.0, -1.0,  1.0),
    vec3(-1.0, -1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3(-1.0, -1.0,  1.0),

    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),

    vec3(-1.0, -1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3(-1.0, -1.0,  1.0),

    vec3(-1.0,  1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3(-1.0,  1.0, -1.0),

    vec3(-1.0, -1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0)
);

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(location = 0) out vec3 vPosition;

void main()
{
    vPosition = SkyboxVertices[gl_VertexIndex];
    gl_Position = uViewProjection * vec4(10000.0 * vPosition + uCameraPosition, 1.0);
}