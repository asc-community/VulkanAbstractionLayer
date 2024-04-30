#version 460
layout(quads, equal_spacing, cw) in;

layout(location = 0) in vec3 iPosition[];
layout(location = 1) in vec2 iTexCoord[];
layout(location = 2) in flat uint iMaterialIndex[];
layout(location = 3) in mat3 iNormalMatrix[];

layout(location = 0) out vec3 oPosition;
layout(location = 1) out vec2 oTexCoord;
layout(location = 2) out flat uint oMaterialIndex;
layout(location = 3) out mat3 oNormalMatrix;

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

float hash(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * 10.21);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}
float noise (vec2 P)
{
    float size = 256.0;
    float s = (1.0 / size);
    vec2 pixel = P * size + 0.5;   
    vec2 f = fract(pixel);
    pixel = (floor(pixel) / size) - vec2(s/2.0, s/2.0);
    float C11 = hash(pixel + vec2( 0.0, 0.0));
    float C21 = hash(pixel + vec2( s, 0.0));
    float C12 = hash(pixel + vec2( 0.0, s));
    float C22 = hash(pixel + vec2( s, s));
    float x1 = mix(C11, C21, f.x);
    float x2 = mix(C12, C22, f.x);
    return mix(x1, x2, f.y);
}
float fbm( vec2 p )
{
    float a = 0.5, b = 0.0, t = 0.0;
    for (int i=0; i<5; i++)
    {
        b *= a; t *= a;
        b += noise(p);
        t += 1.0; p /= 2.0;
    }
    return b /= t;
}

void main()
{
	vec2 uv1 = mix(iTexCoord[0], iTexCoord[1], gl_TessCoord.x);
	vec2 uv2 = mix(iTexCoord[3], iTexCoord[2], gl_TessCoord.x);
	oTexCoord = mix(uv1, uv2, gl_TessCoord.y);

	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(pos1, pos2, gl_TessCoord.y);
    pos.z -= 70.0 * fbm(oTexCoord);
    oPosition = uModel * pos.xyz;
    gl_Position = uViewProjection * vec4(oPosition, 1.0);
    
    oMaterialIndex = iMaterialIndex[0];
    oNormalMatrix = iNormalMatrix[0];
}


