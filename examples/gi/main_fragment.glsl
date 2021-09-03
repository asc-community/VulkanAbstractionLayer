#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in mat3 vNormalMatrix;

layout(location = 0) out vec4 oColor;

struct Material
{
    uint AlbedoTextureIndex;
    uint NormalTextureIndex;
    uint MetallicRoughnessTextureIndex;
    float RoughnessScale;
};

struct Fragment
{
    vec3 Albedo;
    vec3 Normal;
    float Metallic;
    float Roughness;
};

layout(set = 0, binding = 3) uniform uMaterialBuffer
{
    Material uMaterials[256];
};

layout(set = 0, binding = 4) uniform texture2D uTextures[4096];
layout(set = 0, binding = 5) uniform sampler uImageSampler;

layout(set = 0, binding = 6) uniform sampler2D uBRDFLUT;
layout(set = 0, binding = 7) uniform samplerCube uSkybox;
layout(set = 0, binding = 8) uniform samplerCube uSkyboxIrradiance;

layout(push_constant) uniform uPushConstant
{
    vec3 uCameraPosition;
    uint uMaterialIndex;
};

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

void main() 
{
    Material material = uMaterials[uMaterialIndex];
    vec4 albedoColor   = texture(sampler2D(uTextures[material.AlbedoTextureIndex], uImageSampler), vTexCoord);
    vec4 normalColor   = texture(sampler2D(uTextures[material.NormalTextureIndex], uImageSampler), vTexCoord);
    vec4 metallicRoughnessColor = texture(sampler2D(uTextures[material.MetallicRoughnessTextureIndex], uImageSampler), vTexCoord);
    
    if(albedoColor.a < 0.5)
        discard;

    vec3 viewDirection = normalize(uCameraPosition - vPosition);

    Fragment fragment;
    fragment.Albedo = pow(albedoColor.rgb, vec3(GAMMA));
    fragment.Normal = vNormalMatrix * vec3(2.0 * normalColor.rgb - 1.0);
    fragment.Metallic = 1.0 - metallicRoughnessColor.b;
    fragment.Roughness = material.RoughnessScale * metallicRoughnessColor.g;

    vec3 color = calculateIBL(fragment, viewDirection, uSkybox, uSkyboxIrradiance, uBRDFLUT);
    oColor = vec4(pow(color, vec3(1.0 / GAMMA)), 1.0);
}