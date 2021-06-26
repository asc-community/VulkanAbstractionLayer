#version 460

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in flat uint vAlbedoTextureIndex;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 2) uniform uLightBuffer
{
    vec4 uLightColorPadding;
    vec3 uLightDirection;
};

layout(set = 0, binding = 3) uniform sampler uImageSampler;
layout(set = 0, binding = 4) uniform texture2D uTextures[4096];

void main() 
{
    vec3 lightColor = uLightColorPadding.rgb;
    vec3 albedoColor = texture(sampler2D(uTextures[vAlbedoTextureIndex], uImageSampler), vTexCoord).rgb;
    float diffuseFactor = max(dot(uLightDirection, vNormal), 0.0);
    oColor = vec4(lightColor * albedoColor * diffuseFactor, 1.0);
}