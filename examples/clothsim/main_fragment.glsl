#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform uColorPushConstants
{
    vec3 uColor;
};

void main() 
{
    const vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuseFactor = dot(vNormal, lightDir);
    oColor = vec4(diffuseFactor * uColor, 1.0);
}