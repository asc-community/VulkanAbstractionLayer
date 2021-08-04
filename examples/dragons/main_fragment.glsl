#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in flat uint vAlbedoTextureIndex;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(set = 0, binding = 2) uniform uLightBuffer
{
    mat4 uLightProjection;
    vec4 uLightColor_uAmbientIntensity;
    vec3 uLightDirection;
};

layout(set = 0, binding = 3) uniform sampler uImageSampler;
layout(set = 0, binding = 4) uniform texture2D uTextures[4096];

layout(set = 0, binding = 5) uniform sampler2D uShadowTexture;

float isInShadow(vec2 projectedShadowUV, float depth, sampler2D shadowMap)
{
    const float bias = 0.02;
    float compareDepth = texture(shadowMap, projectedShadowUV).r;
    return depth < compareDepth + bias ? 1.0 : 0.0;
}

float computeShadowPCF(vec3 position, mat4 lightProjection, sampler2D shadowMap, int kernelSize, float kernelOfffset)
{
    vec4 positionLightSpace = lightProjection * vec4(position, 1.0);
    vec3 projectedPosition = positionLightSpace.xyz / positionLightSpace.w;
    projectedPosition.xy = vec2(0.5, -0.5) * projectedPosition.xy + 0.5;

    if (projectedPosition.x < 0.0 || projectedPosition.x > 1.0 ||
        projectedPosition.y < 0.0 || projectedPosition.y > 1.0 ||
        projectedPosition.z < 0.0 || projectedPosition.z > 1.0) return 1.0;

    vec2 invShadowMapSize = vec2(kernelOfffset) / textureSize(shadowMap, 0);

    float accum = 0.0;
    int totalSamples = (2 * kernelSize + 1) * (2 * kernelSize + 1);
    for (int x = -kernelSize; x <= kernelSize; x++)
    {
        for (int y = -kernelSize; y <= kernelSize; y++)
        {
            accum += isInShadow(projectedPosition.xy + vec2(x, y) * invShadowMapSize, projectedPosition.z, shadowMap);
        }
    }

    return accum / float(totalSamples);
}

float computeShadow(vec3 position, mat4 lightProjection, sampler2D shadowMap)
{
    float initialTest = computeShadowPCF(position, lightProjection, shadowMap, 1, 5.0);
    if (initialTest == 0.0 || initialTest == 1.0) return initialTest;
    return computeShadowPCF(position, lightProjection, shadowMap, 5, 1.0);
}

void main() 
{
    vec3 albedoColor = texture(sampler2D(uTextures[vAlbedoTextureIndex], uImageSampler), vTexCoord).rgb;
    float shadowFactor = computeShadow(vPosition, uLightProjection, uShadowTexture);

    vec3 cameraDirection = normalize(uCameraPosition - vPosition);
    vec3 H = normalize(cameraDirection + uLightDirection);
    float specularFactor = pow(max(dot(H, vNormal), 0.0), 400.0);
    float diffuseFactor = max(dot(uLightDirection, vNormal), 0.0);
    float ambientFactor = uLightColor_uAmbientIntensity.a;
    float totalFactor = shadowFactor * (diffuseFactor + specularFactor) + ambientFactor;

    vec3 lightColor = uLightColor_uAmbientIntensity.rgb;
    oColor = vec4(totalFactor * lightColor * albedoColor, 1.0);
}