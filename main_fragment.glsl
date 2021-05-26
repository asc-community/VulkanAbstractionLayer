#version 460

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 oColor;

void main() 
{
    oColor = vec4(vTexCoord.xy, 0.2, 1.0);
}