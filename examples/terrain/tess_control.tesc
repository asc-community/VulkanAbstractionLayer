#version 460

layout(location = 0) in vec3 iPosition[];
layout(location = 1) in vec2 iTexCoord[];
layout(location = 2) in flat uint iMaterialIndex[];
layout(location = 3) in mat3 iNormalMatrix[];

layout (vertices = 4) out;
layout(location = 0) out vec3 oPosition[4];
layout(location = 1) out vec2 oTexCoord[4];
layout(location = 2) out flat uint oMaterialIndex[4];
layout(location = 3) out mat3 oNormalMatrix[4];

layout(set = 0, binding = 0) uniform uCameraBuffer
{
	mat4 uViewProjection;
	mat4 uView;
	vec3 uCameraPosition;
};
layout(set = 0, binding = 1) uniform uModelBuffer
{
    mat3 uModel;
};

#define MIN_DISTANCE   5
#define MAX_DISTANCE   150
#define MIN_TESS_LEVEL 1
#define MAX_TESS_LEVEL 16

float getDist(vec4 ipos)
{
	vec4 world = vec4(uModel * ipos.xyz, 1.0);
	vec4 eye = uView * world;
	float dist = clamp((abs(eye.z)-MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0);
	return dist;
}

void main()
{
	if (gl_InvocationID == 0)
	{
		float dist00 = getDist(gl_in[0].gl_Position);
		float dist01 = getDist(gl_in[1].gl_Position);
		float dist10 = getDist(gl_in[2].gl_Position);
		float dist11 = getDist(gl_in[3].gl_Position);

		float tessLv0 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dist10, dist00) );
		float tessLv1 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dist00, dist01) );
		float tessLv2 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dist01, dist11) );
		float tessLv3 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(dist11, dist10) );

		gl_TessLevelOuter[0] = tessLv0;
		gl_TessLevelOuter[1] = tessLv1;
		gl_TessLevelOuter[2] = tessLv2;
		gl_TessLevelOuter[3] = tessLv3;

		gl_TessLevelInner[0] = max(tessLv1, tessLv3);
		gl_TessLevelInner[1] = max(tessLv0, tessLv2);	
    }	
	gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
	oPosition[gl_InvocationID] = iPosition[gl_InvocationID];
	oTexCoord[gl_InvocationID] = iTexCoord[gl_InvocationID];
	oMaterialIndex[gl_InvocationID] = iMaterialIndex[gl_InvocationID];
	oNormalMatrix[gl_InvocationID] = iNormalMatrix[gl_InvocationID];
}
