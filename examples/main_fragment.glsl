#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in flat uint vAlbedoTextureIndex;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 2) uniform uLightBuffer
{
    mat4 uLightProjection;
    vec4 uLightColorPadding;
    vec3 uLightDirection;
};

layout(set = 0, binding = 3) uniform sampler uImageSampler;
layout(set = 0, binding = 4) uniform texture2D uTextures[4096];

layout(set = 0, binding = 5) uniform texture2D uShadowTexture;
layout(set = 0, binding = 6) uniform sampler uShadowSampler;

float computeShadow(vec3 position, mat4 lightProjection, texture2D shadowMap, sampler depthSampler)
{
    const float bias = 0.001;
    vec4 positionLightSpace = lightProjection * vec4(position, 1.0);
    vec3 projectedPosition = positionLightSpace.xyz / positionLightSpace.w;
    float compareDepth = texture(sampler2D(shadowMap, depthSampler), projectedPosition.xy).r;
    return step(projectedPosition.z, compareDepth);
}

void main() 
{
    vec3 lightColor = uLightColorPadding.rgb;
    float shadowFactor = computeShadow(vPosition, uLightProjection, uShadowTexture, uShadowSampler);
    
    vec3 albedoColor = texture(sampler2D(uTextures[vAlbedoTextureIndex], uImageSampler), vTexCoord).rgb;
    float diffuseFactor = max(dot(uLightDirection, vNormal), 0.0);
    oColor = vec4(shadowFactor * lightColor * albedoColor * diffuseFactor, 1.0);

    vec4 positionLightSpace = uLightProjection * vec4(vPosition, 1.0);
    vec3 projectedPosition = positionLightSpace.xyz / positionLightSpace.w;
    float compareDepth = texture(sampler2D(uShadowTexture, uShadowSampler), projectedPosition.xy).r;
    oColor = vec4(vec3(compareDepth), 1.0);
}