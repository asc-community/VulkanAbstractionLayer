#version 460

layout (local_size_x = 256) in;

struct InstanceData
{
    vec4 Position;
};

layout(set = 0, binding = 0) buffer uComputeBuffer {
    InstanceData uInstances[];
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
        uInstances[id].Position.x -= 0.1 * float(id) * uTimeDelta;
    }
}