#version 460

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

void main() 
{
    const vec3 light = normalize(vec3(1.0, 1.0, 0.0));
    float diffuseFactor = max(dot(light, vNormal), 0.0);
    oColor = vec4(vec3(diffuseFactor), 1.0);
}