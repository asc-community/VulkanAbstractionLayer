#version 460

layout (local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rgba32f) uniform image2D uPositionImage;
layout(set = 0, binding = 1, rgba32f) uniform image2D uVelocityImage;

layout(push_constant) uniform uComputeShaderInfo
{
    vec4 uNodePosition;
    ivec2 uNodeUV;
};

#define BALL_COUNT 2
layout(set = 0, binding = 2) uniform uBallBuffer
{
    vec4 uBallPosition_BallRadius[BALL_COUNT];
};

const float mass = 1.0;
const vec3 gravity = vec3(0.0, -9.8, 0.0);
const float damping = -0.5;
const float naturalLength = 1.0;
const float stiffness = 10000.0;
const float timeDelta = 1.0 / 300.0;

vec4 uPositionImageLoadBoundCheck(ivec2 uv, vec4 defaultValue)
{
    ivec2 size = imageSize(uPositionImage);
    if (uv.x < 0 || uv.y < 0 || uv.x >= size.x || uv.y >= size.y)
        return defaultValue;
    return imageLoad(uPositionImage, uv);
}

void main()
{
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    ivec2 size = imageSize(uPositionImage);

    bool isControlNode = (x == 0 || x == size.x - 1) && (y == 0 || y == size.y - 1);
    bool isUserSelectedNode = ivec2(x, y) == uNodeUV;

    if(x < size.x && y < size.y)
    {
        vec4 position = imageLoad(uPositionImage, ivec2(x, y));
        vec4 velocity = imageLoad(uVelocityImage, ivec2(x, y));

        vec3 forceGravity = mass * gravity;
        vec3 forceDamping = damping * velocity.xyz;

        vec3 forceInteraction = vec3(0.0);
        for(int i = 0; i < BALL_COUNT; i++)
        {
            vec3 ballRelativePosition = position.xyz - uBallPosition_BallRadius[i].xyz;
            forceInteraction += stiffness * max(uBallPosition_BallRadius[i].w - length(ballRelativePosition), 0.0) * normalize(ballRelativePosition);
        }

        vec4 neighborRelativePositions[4];
        neighborRelativePositions[0] = uPositionImageLoadBoundCheck(ivec2(x - 1, y), position + vec4(0.001)) - position;
        neighborRelativePositions[1] = uPositionImageLoadBoundCheck(ivec2(x + 1, y), position + vec4(0.001)) - position;
        neighborRelativePositions[2] = uPositionImageLoadBoundCheck(ivec2(x, y - 1), position + vec4(0.001)) - position;
        neighborRelativePositions[3] = uPositionImageLoadBoundCheck(ivec2(x, y + 1), position + vec4(0.001)) - position;

        vec3 forceSpring = stiffness * (
            max(length(neighborRelativePositions[0].xyz) - naturalLength, 0.0) * normalize(neighborRelativePositions[0].xyz) +
            max(length(neighborRelativePositions[1].xyz) - naturalLength, 0.0) * normalize(neighborRelativePositions[1].xyz) +
            max(length(neighborRelativePositions[2].xyz) - naturalLength, 0.0) * normalize(neighborRelativePositions[2].xyz) +
            max(length(neighborRelativePositions[3].xyz) - naturalLength, 0.0) * normalize(neighborRelativePositions[3].xyz)
        );

        vec3 forceTotal = forceSpring + forceGravity + forceInteraction + forceDamping;
        vec3 acceleration = forceTotal / mass;

        velocity.xyz += acceleration * timeDelta;

        if (!isControlNode)
            position.xyz += velocity.xyz * timeDelta;

        if (isUserSelectedNode)
            position.xyz = uNodePosition.xyz;

        imageStore(uVelocityImage, ivec2(x, y), velocity);
        imageStore(uPositionImage, ivec2(x, y), position);
    }
}