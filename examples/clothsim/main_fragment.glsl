#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform uColorPushConstants
{
    vec4 uColor_QuadsPerRow;
};

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

void main() 
{
    const vec3 lightDirection = normalize(vec3(-0.5, 1.0, -0.3));
    const float ambientFactor = 0.1;
    vec3 cameraDirection = normalize(uCameraPosition - vPosition);
    vec3 H = normalize(cameraDirection + lightDirection);

    float specularFactor = pow(max(dot(H, vNormal), 0.0), 256.0);
    float diffuseFactor = max(dot(lightDirection, vNormal), 0.0);
    float totalFactor = diffuseFactor + specularFactor + ambientFactor;
    vec3 color = uColor_QuadsPerRow.rgb;
    oColor = vec4(totalFactor * color, 1.0);
}