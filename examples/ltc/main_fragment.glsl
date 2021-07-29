#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

layout(set = 0, binding = 2) uniform uLightBuffer
{
    vec4 uLightColor_uAmbientIntensity;
    vec3 uLightDirection;
};

void main() 
{
    vec3 albedoColor = vec3(1.0);
    vec3 cameraDirection = normalize(uCameraPosition - vPosition);
    vec3 H = normalize(cameraDirection + uLightDirection);
    float specularFactor = pow(max(dot(H, vNormal), 0.0), 400.0);
    float diffuseFactor = max(dot(uLightDirection, vNormal), 0.0);
    float ambientFactor = uLightColor_uAmbientIntensity.a;
    float totalFactor = diffuseFactor + specularFactor + ambientFactor;

    vec3 lightColor = uLightColor_uAmbientIntensity.rgb;
    oColor = vec4(totalFactor * lightColor * albedoColor, 1.0);
}