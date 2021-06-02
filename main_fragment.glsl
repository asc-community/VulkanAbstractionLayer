#version 460

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 2) uniform uLightBuffer
{
    vec4 uLightColorPadding;
    vec3 uLightDirection;
};

void main() 
{
    vec3 lightColor = uLightColorPadding.rgb;
    float diffuseFactor = max(dot(uLightDirection, vNormal), 0.0);
    oColor = vec4(lightColor * diffuseFactor, 1.0);
}