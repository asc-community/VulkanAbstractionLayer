#version 460

layout(location = 0) in vec4 iPosition;
layout(location = 1) in vec2 iTexCoord;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec2 vTexCoord;

void main() 
{
    gl_Position = iPosition;
    vTexCoord = iTexCoord; 
}