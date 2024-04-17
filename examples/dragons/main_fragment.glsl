#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in flat uint vMaterialIndex;
layout(location = 3) in mat3 vNormalMatrix;

layout(location = 0) out vec4 oColor;

struct Material
{
    uint AlbedoTextureIndex;
    uint NormalTextureIndex;
    float MetallicFactor;
    float RoughnessFactor;
};

struct Fragment
{
    vec3 Albedo;
    vec3 Normal;
    float Metallic;
    float Roughness;
};

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

layout(set = 0, binding = 3) uniform uMaterialBuffer
{
    Material uMaterials[256];
};

layout(set = 0, binding = 4) uniform sampler uImageSampler;
layout(set = 0, binding = 5) uniform texture2D uTextures[64];

layout(set = 0, binding = 6) uniform sampler2D uShadowTexture;
layout(set = 0, binding = 7) uniform sampler2D uBRDFLUT;
layout(set = 0, binding = 8) uniform samplerCube uSkybox;
layout(set = 0, binding = 9) uniform samplerCube uSkyboxIrradiance;

#define PI 3.1415926535
#define GAMMA 2.2

// float GGXPartialGeometry(float NV, float roughness2)
// {
//     return NV / mix(NV, 1.0, roughness2);
// }
// 
// float GGXDistribution(float NH, float roughness)
// {
//     float roughness2 = roughness * roughness;
//     float alpha2 = roughness2 * roughness2;
//     float distr = (NH * NH) * (alpha2 - 1.0) + 1.0;
//     float distr2 = distr * distr;
//     float totalDistr = alpha2 / (PI * distr2);
//     return totalDistr;
// }
// 
// float GGXSmith(float NV, float NL, float roughness)
// {
//     float d = roughness * 0.125 + 0.125;
//     float roughness2 = roughness * d + d;
//     return GGXPartialGeometry(NV, roughness2) * GGXPartialGeometry(NL, roughness2);
// }
// 
// vec3 fresnelSchlick(vec3 F0, float HV)
// {
//     vec3 fresnel = F0 + (1.0 - F0) * pow(2.0, (-5.55473 * HV - 6.98316) * HV);
//     return fresnel;
// }
// 
// void GGXCookTorranceSampled(vec3 normal, vec3 lightDirection, vec3 viewDirection, float roughness, float metallic, vec3 albedo,
//     out vec3 specular, out vec3 diffuse)
// {
//     vec3 H = normalize(viewDirection + lightDirection);
//     float LV = dot(lightDirection, viewDirection);
//     float NV = dot(normal, viewDirection);
//     float NL = dot(normal, lightDirection);
//     float NH = dot(normal, H);
//     float HV = dot(H, viewDirection);
// 
//     if (NV < 0.0 || NL < 0.0)
//     {
//         specular = vec3(0.0);
//         diffuse = vec3(0.0);
//         return;
//     }
// 
//     vec3 F0 = mix(vec3(0.04), albedo, metallic);
// 
//     float G = GGXSmith(NV, NL, roughness);
//     float D = GGXDistribution(NH, roughness);
//     vec3 F = fresnelSchlick(F0, HV);
// 
//     specular = D * F * G / (4.0 * NL * NV);
//     specular = clamp(specular, vec3(0.0), vec3(1.0));
// 
//     float s = max(LV, 0.0) - NL * NV;
//     float t = mix(1.0, max(NL, NV), step(0.0, s));
//     float d = 1.0 - metallic;
// 
//     float sigma2 = roughness * roughness;
//     float A = 1.0 + sigma2 * (d / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
//     float B = 0.45 * sigma2 / (sigma2 + 0.09);
// 
//     diffuse = albedo * NL * (1.0 - F) * (A + B * s / t) / PI;
// }

vec3 fresnelSchlickRoughness(vec3 F0, float HV, float roughness)
{
    vec3 fresnel = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(2.0, (-5.55473 * HV - 6.98316) * HV);
    return fresnel;
}

float getTextureLodLevel(vec2 uv)
{
    vec2 dxVtc = dFdx(uv);
    vec2 dyVtc = dFdy(uv);
    float deltaMax2 = max(dot(dxVtc, dxVtc), dot(dyVtc, dyVtc));
    return 0.5 * log2(deltaMax2);
}

vec3 calcReflectionColor(samplerCube reflectionMap, vec3 viewDirection, vec3 normal, float lod)
{
    vec3 I = -viewDirection;
    vec3 reflectionRay = reflect(I, normal);
    reflectionRay = dot(viewDirection, normal) < 0.0 ? -reflectionRay : reflectionRay;

    float defaultLod = getTextureLodLevel(reflectionRay.xy);
    vec3 color = textureLod(reflectionMap, reflectionRay, max(lod, defaultLod)).rgb;
    return color;
}

vec3 calculateIBL(Fragment fragment, vec3 viewDirection, samplerCube env, samplerCube envIrradiance, sampler2D envBRDFLUT)
{
    float roughness = clamp(fragment.Roughness, 0.05, 0.95);
    float metallic = clamp(fragment.Metallic, 0.05, 0.95);

    float NV = clamp(dot(fragment.Normal, viewDirection), 0.0, 0.999);
    
    float lod = log2(textureSize(env, 0).x * roughness * roughness);
    vec3 F0 = mix(vec3(0.04), fragment.Albedo, metallic);
    vec3 F = fresnelSchlickRoughness(F0, NV, roughness);
    
    vec3 prefilteredColor = calcReflectionColor(env, viewDirection, fragment.Normal, lod);
    prefilteredColor = pow(prefilteredColor, vec3(GAMMA));
    vec2 envBRDF = texture(envBRDFLUT, vec2(NV, 1.0 - roughness)).rg;
    vec3 specularColor = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 irradianceColor = calcReflectionColor(envIrradiance, viewDirection, fragment.Normal, 0.0);
    irradianceColor = pow(irradianceColor, vec3(GAMMA));
    
    float diffuseCoef = 1.0 - metallic;
    vec3 diffuseColor = fragment.Albedo * (irradianceColor - irradianceColor * envBRDF.y) * diffuseCoef;
    vec3 iblColor = diffuseColor + specularColor;

    return iblColor;
}

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
    Material material = uMaterials[vMaterialIndex];
    vec3 albedoColor = texture(sampler2D(uTextures[material.AlbedoTextureIndex], uImageSampler), vTexCoord).rgb;
    vec3 normalColor = texture(sampler2D(uTextures[material.NormalTextureIndex], uImageSampler), vTexCoord).rgb;
    
    vec3 viewDirection = normalize(uCameraPosition - vPosition);
    vec3 lightColor = pow(uLightColor_uAmbientIntensity.rgb, vec3(GAMMA));

    Fragment fragment;
    fragment.Albedo = pow(albedoColor, vec3(GAMMA));
    fragment.Normal = vNormalMatrix * vec3(2.0 * normalColor - 1.0);
    fragment.Metallic = material.MetallicFactor;
    fragment.Roughness = material.RoughnessFactor;

    float ambientFactor = uLightColor_uAmbientIntensity.a;
    float diffuseFactor = max(dot(fragment.Normal, uLightDirection), 0.0);

    vec3 color = calculateIBL(fragment, viewDirection, uSkybox, uSkyboxIrradiance, uBRDFLUT);
    float shadowFactor = computeShadow(vPosition, uLightProjection, uShadowTexture);

    vec3 shadowedColor = (ambientFactor + diffuseFactor * shadowFactor) * pow(color, vec3(1.0 / GAMMA));
    oColor = vec4(shadowedColor, 1.0);
}