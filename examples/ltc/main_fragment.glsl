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
    float Metallic;
    float Roughness;
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

void main() 
{
    Material material = uMaterials[uMaterialIndex];
    vec3 albedoColor = texture(sampler2D(uTextures[material.AlbedoIndex], uTextureSampler), vTexCoord).rgb;
    vec3 normalColor = texture(sampler2D(uTextures[material.NormalIndex], uTextureSampler), vTexCoord).rgb;
    vec3 normal = vNormalMatrix * vec3(2.0 * normalColor - 1.0);

    vec3 cameraDirection = normalize(uCameraPosition - vPosition);
    vec3 H = normalize(cameraDirection + uLightDirection);
    float specularFactor = pow(max(dot(H, normal), 0.0), 400.0);
    float diffuseFactor = max(dot(uLightDirection, normal), 0.0);
    float ambientFactor = uLightColor_uAmbientIntensity.a;
    float totalFactor = diffuseFactor + specularFactor + ambientFactor;

    vec3 lightColor = uLightColor_uAmbientIntensity.rgb;
    oColor = vec4(totalFactor * lightColor * albedoColor, 1.0);
}