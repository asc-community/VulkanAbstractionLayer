#version 460

layout(location = 0) in vec3 vPosition;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 8) uniform samplerCube uSkyboxCubemap;

void main()
{
    oColor = vec4(texture(uSkyboxCubemap, normalize(vPosition)).rgb, 1.0);
}