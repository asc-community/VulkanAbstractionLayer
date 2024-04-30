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
	mat4 uView;
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
    float shadowFactor = 1.0;
    vec3 shadowedColor = (ambientFactor + diffuseFactor * shadowFactor) * pow(color, vec3(1.0 / GAMMA));
    oColor = vec4(shadowedColor, 1.0);
}