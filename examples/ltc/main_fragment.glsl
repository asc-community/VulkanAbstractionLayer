#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in mat3 vNormalMatrix;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

struct Material
{
    uint AlbedoIndex;
    uint NormalIndex;
    uint MetallicRoughnessIndex;
    uint stub;
};

layout(set = 0, binding = 2) uniform uMaterialArray
{
    Material uMaterials[256];
};

layout(push_constant) uniform uMaterialConstant
{
    uint uMaterialIndex;
};

layout(set = 0, binding = 3) uniform uLightBuffer
{
    vec4 uLightColor_uAmbientIntensity;
    vec3 uLightDirection;
};

layout(set = 0, binding = 4) uniform texture2D uTextures[512];
layout(set = 0, binding = 5) uniform sampler uTextureSampler;

struct Fragment
{
    vec3 Albedo;
    vec3 Normal;
    float Metallic;
    float Roughness;
};

#define PI 3.1415926535f

float GGXPartialGeometry(float NV, float roughness2)
{
    return NV / mix(NV, 1.0, roughness2);
}

float GGXDistribution(float NH, float roughness)
{
    float roughness2 = roughness * roughness;
    float alpha2 = roughness2 * roughness2;
    float distr = (NH * NH) * (alpha2 - 1.0f) + 1.0f;
    float distr2 = distr * distr;
    float totalDistr = alpha2 / (PI * distr2);
    return totalDistr;
}

float GGXSmith(float NV, float NL, float roughness)
{
    float d = roughness * 0.125 + 0.125;
    float roughness2 = roughness * d + d;
    return GGXPartialGeometry(NV, roughness2) * GGXPartialGeometry(NL, roughness2);
}

vec3 fresnelSchlick(vec3 F0, float HV)
{
    vec3 fresnel = F0 + (1.0 - F0) * pow(2.0, (-5.55473 * HV - 6.98316) * HV);
    return fresnel;
}

void GGXCookTorranceSampled(vec3 normal, vec3 lightDirection, vec3 viewDirection, float roughness, float metallic, vec3 albedo,
    out vec3 specular, out vec3 diffuse)
{
    vec3 H = normalize(viewDirection + lightDirection);
    float LV = dot(lightDirection, viewDirection);
    float NV = dot(normal, viewDirection);
    float NL = dot(normal, lightDirection);
    float NH = dot(normal, H);
    float HV = dot(H, viewDirection);

    if (NV < 0.0 || NL < 0.0)
    {
        specular = vec3(0.0);
        diffuse = vec3(0.0);
        return;
    }

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float G = GGXSmith(NV, NL, roughness);
    float D = GGXDistribution(NH, roughness);
    vec3 F = fresnelSchlick(F0, HV);

    specular = D * F * G / (4.0 * NL * NV);
    specular = clamp(specular, vec3(0.0), vec3(1.0));

    float s = max(LV, 0.0) - NL * NV;
    float t = mix(1.0, max(NL, NV), step(0.0, s));
    float d = 1.0 - metallic;

    float sigma2 = roughness * roughness;
    float A = 1.0 + sigma2 * (d / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    diffuse = albedo * NL * (1.0 - F) * (A + B * s / t) / PI;
}

vec3 calculateLighting(Fragment fragment, vec3 viewDirection, vec3 lightDirection, vec3 lightColor, float ambientFactor)
{
    float roughness = clamp(fragment.Roughness, 0.05, 0.95);
    float metallic = clamp(fragment.Metallic, 0.05, 0.95);

    vec3 specularColor = vec3(0.0);
    vec3 diffuseColor = vec3(0.0);
    GGXCookTorranceSampled(fragment.Normal, lightDirection, viewDirection, roughness, metallic, fragment.Albedo,
        specularColor, diffuseColor);

    vec3 ambientColor = diffuseColor * ambientFactor;
    vec3 totalColor = (ambientColor + diffuseColor + specularColor) * lightColor;
    return totalColor;
}

void main() 
{
    Material material = uMaterials[uMaterialIndex];
    vec4 albedoColor = texture(sampler2D(uTextures[material.AlbedoIndex], uTextureSampler), vTexCoord).rgba;
    vec3 normalColor = texture(sampler2D(uTextures[material.NormalIndex], uTextureSampler), vTexCoord).rgb;
    vec3 metallicRoughnessColor = texture(sampler2D(uTextures[material.MetallicRoughnessIndex], uTextureSampler), vTexCoord).rgb;

    if (albedoColor.a < 0.5)
        discard;

    Fragment fragment;
    fragment.Albedo = albedoColor.rgb;
    fragment.Normal = vNormalMatrix * vec3(2.0 * normalColor - 1.0);
    fragment.Metallic = metallicRoughnessColor.b;
    fragment.Roughness = metallicRoughnessColor.g;

    vec3 viewDirection = normalize(uCameraPosition - vPosition); 

    vec3 totalColor = calculateLighting(fragment, viewDirection, uLightDirection, uLightColor_uAmbientIntensity.rgb, uLightColor_uAmbientIntensity.a);

    oColor = vec4(totalColor, 1.0);
}