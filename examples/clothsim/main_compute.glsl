#version 460

layout (local_size_x = 256) in;

layout(set = 0, binding = 0) buffer uComputeBuffer{
    vec3 uPositions[];
};

layout(push_constant) uniform uComputeShaderInfo
{
    uint uPositionCount;
    float uTimeDelta;
};

void main()
{
    uint id = gl_GlobalInvocationID.x;

    if(id < uPositionCount)
    {
        vec3 position = uPositions[id];
        position.x -= float(id) * uTimeDelta;
        uPositions[id] = position;
    }
}