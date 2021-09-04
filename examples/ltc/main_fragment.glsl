#version 460

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in mat3 vNormalMatrix;

layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform uCameraBuffer
{
    mat4 uViewProjection;
    vec3 uCameraPosition;
};

struct Material
{
    uint AlbedoIndex;
    uint NormalIndex;
    uint MetallicRoughnessIndex;
    float RoughnessScale;
};

layout(set = 0, binding = 2) uniform uMaterialArray
{
    Material uMaterials[256];
};

layout(push_constant) uniform uMaterialConstant
{
    uint uMaterialIndex;
};

struct LightData
{
    mat3 Rotation;
    vec4 Position_Width;
    vec4 Color_Height;
    uint TextureIndex;
};

#define LIGHT_COUNT 4
#define MATERIAL_TEXTURE_COUNT 512

layout(set = 0, binding = 3) uniform uLightArray
{
    LightData uLights[LIGHT_COUNT];
};

layout(set = 0, binding = 4) uniform texture2D uTextures[MATERIAL_TEXTURE_COUNT];
layout(set = 0, binding = 5) uniform sampler uTextureSampler;
layout(set = 0, binding = 6) uniform sampler2D uLookupLTCMatrix;
layout(set = 0, binding = 7) uniform sampler2D uLookupLTCAmplitude;
layout(set = 0, binding = 8) uniform texture2D uLightTextures[LIGHT_COUNT];

struct Fragment
{
    vec3 Albedo;
    vec3 Normal;
    float Metallic;
    float Roughness;
};

#define PI 3.1415926535
#define GAMMA 2.2

struct Ray
{
    vec3 origin;
    vec3 dir;
};

struct Rect
{
    vec3  center;
    vec3  dirx;
    vec3  diry;
    float halfx;
    float halfy;

    vec4  plane;
};

vec3 FetchDiffuseFilteredTexture(texture2D texLightFiltered, sampler texLightSampler, vec3 p1_, vec3 p2_, vec3 p3_, vec3 p4_)
{
    // area light plane basis
    vec3 V1 = p2_ - p1_;
    vec3 V2 = p4_ - p1_;
    vec3 planeOrtho = (cross(V1, V2));
    float planeAreaSquared = dot(planeOrtho, planeOrtho);
    float planeDistxPlaneArea = dot(planeOrtho, p1_);
    // orthonormal projection of (0,0,0) in area light space
    vec3 P = planeDistxPlaneArea * planeOrtho / planeAreaSquared - p1_;

    // find tex coords of P
    float dot_V1_V2 = dot(V1, V2);
    float inv_dot_V1_V1 = 1.0 / dot(V1, V1);
    vec3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
    vec2 Puv;
    Puv.y = dot(V2_, P) / dot(V2_, V2_);
    Puv.x = dot(V1, P) * inv_dot_V1_V1 - dot_V1_V2 * inv_dot_V1_V1 * Puv.y;

    // LOD
    float d = abs(planeDistxPlaneArea) / pow(planeAreaSquared, 0.75);
    vec2 size = textureSize(sampler2D(texLightFiltered, texLightSampler), 0);

    return textureLod(sampler2D(texLightFiltered, texLightSampler), vec2(0.125, 0.125) + 0.75 * Puv, log(size.x * d) / log(3.0)).rgb;
}

vec3 FetchDiffuseTexture(texture2D texLightFiltered, sampler texLightSampler, vec2 uv)
{
    return texture(sampler2D(texLightFiltered, texLightSampler), 0.75 * uv + 0.125, -1.25).rgb;
}

bool RayPlaneIntersect(Ray ray, vec4 plane, out float t)
{
    t = -dot(plane, vec4(ray.origin, 1.0)) / dot(plane.xyz, ray.dir);
    return t > 0.0;
}

bool RayRectIntersect(Ray ray, Rect rect, out vec2 uv, out float t)
{
    bool intersect = RayPlaneIntersect(ray, rect.plane, t);
    uv = vec2(0.0);
    if (intersect)
    {
        vec3 pos = ray.origin + ray.dir * t;
        vec3 lpos = pos - rect.center;

        float x = dot(lpos, rect.dirx);
        float y = dot(lpos, rect.diry);

        if (abs(x) > rect.halfx || abs(y) > rect.halfy)
            intersect = false;
        else
            uv = 0.5 * vec2(x, y) / vec2(rect.halfx, rect.halfy) + 0.5;
    }

    return intersect;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
    float cosTheta = dot(v1, v2);
    cosTheta = clamp(cosTheta, -0.9999, 0.9999);

    float theta = acos(cosTheta);    
    float res = cross(v1, v2).z * theta / sin(theta);

    return res;
}

void ClipQuadToHorizon(inout vec3 L[5], out int n)
{
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;

    // clip
    n = 0;

    if (config == 0)
    {
        // clip all
    }
    else if (config == 1) // V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2) // V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3) // V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4) // V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5) // V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6) // V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7) // V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8) // V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = L[3];
    }
    else if (config == 9) // V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10) // V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11) // V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12) // V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13) // V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14) // V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15) // V1 V2 V3 V4
    {
        n = 4;
    }

    if (n == 3)
        L[3] = L[0];
    if (n == 4)
        L[4] = L[0];
}


vec2 LTC_Coords(float cosTheta, float roughness)
{
    float theta = acos(cosTheta);
    vec2 coords = vec2(roughness, theta / (0.5 * PI));

    const float LUT_SIZE = 32.0;
    // scale and bias coordinates, for correct filtered lookup
    coords = coords * (LUT_SIZE - 1.0) / LUT_SIZE + 0.5 / LUT_SIZE;

    return coords;
}


vec3 LTC_Evaluate(
    vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], texture2D texFilteredMap, sampler texSampler, bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));

    // polygon (allocate 5 vertices for clipping)
    vec3 L[5];
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    vec3 lightTexture = FetchDiffuseFilteredTexture(texFilteredMap, texSampler, L[0], L[1], L[2], L[3]);
    lightTexture = pow(lightTexture, vec3(GAMMA));

    int n;
    ClipQuadToHorizon(L, n);

    if (n == 0)
        return vec3(0, 0, 0);

    // project onto sphere
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);
    L[4] = normalize(L[4]);

    // integrate
    float sum = 0.0;

    sum += IntegrateEdge(L[0], L[1]);
    sum += IntegrateEdge(L[1], L[2]);
    sum += IntegrateEdge(L[2], L[3]);
    if (n >= 4)
        sum += IntegrateEdge(L[3], L[4]);
    if (n == 5)
        sum += IntegrateEdge(L[4], L[0]);

    sum = twoSided ? abs(sum) : max(0.0, sum);

    vec3 Lo_i = sum * lightTexture;

    return Lo_i;
}

void InitRect(out Rect rect, vec3 position, mat3 rotation, float width, float height)
{
    rect.dirx = rotation * vec3(1, 0, 0);
    rect.diry = rotation * vec3(0, 1, 0);

    rect.center = position;
    rect.halfx = 0.5 * width;
    rect.halfy = 0.5 * height;

    vec3 rectNormal = cross(rect.dirx, rect.diry);
    rect.plane = vec4(rectNormal, -dot(rectNormal, rect.center));
}

void InitRectPoints(Rect rect, out vec3 points[4])
{
    vec3 ex = rect.halfx * rect.dirx;
    vec3 ey = rect.halfy * rect.diry;

    points[0] = rect.center - ex - ey;
    points[1] = rect.center + ex - ey;
    points[2] = rect.center + ex + ey;
    points[3] = rect.center - ex + ey;
}

void main()
{
    Material material = uMaterials[uMaterialIndex];
    vec4 albedoColor = texture(sampler2D(uTextures[material.AlbedoIndex], uTextureSampler), vTexCoord).rgba;
    vec3 normalColor = texture(sampler2D(uTextures[material.NormalIndex], uTextureSampler), vTexCoord).rgb;
    vec3 metallicRoughnessColor = texture(sampler2D(uTextures[material.MetallicRoughnessIndex], uTextureSampler), vTexCoord).rgb;

    if (albedoColor.a < 0.5)
        discard;

    Fragment fragment;
    fragment.Albedo = pow(albedoColor.rgb, vec3(GAMMA));
    fragment.Normal = vNormalMatrix * vec3(2.0 * normalColor - 1.0);
    fragment.Metallic = metallicRoughnessColor.b;
    fragment.Roughness = material.RoughnessScale * metallicRoughnessColor.g;

    vec3 viewDirection = normalize(uCameraPosition - vPosition);

    vec3 diffuseColor = fragment.Albedo * (1.0 - fragment.Metallic);
    vec3 specularColor = fragment.Albedo;

    Ray ray;
    ray.origin = uCameraPosition;
    ray.dir = -viewDirection;

    const bool twoSided = true;

    vec2 uv = LTC_Coords(dot(fragment.Normal, -viewDirection), fragment.Roughness);

    vec4 t = texture(uLookupLTCMatrix, uv);
    mat3 Minv = mat3(
        vec3(1, 0, t.y),
        vec3(0, t.z, 0),
        vec3(t.w, 0, t.x)
    );
    vec2 schlick = texture(uLookupLTCAmplitude, uv).xy;

    Rect lightRects[LIGHT_COUNT];
    for (int i = 0; i < LIGHT_COUNT; i++)
    {            
        InitRect(lightRects[i], uLights[i].Position_Width.xyz, uLights[i].Rotation, uLights[i].Position_Width.w, uLights[i].Color_Height.w);
    }

    vec3 totalColor = vec3(0.0);

    Ray reflectRay;
    reflectRay.origin = vPosition;
    reflectRay.dir = reflect(-viewDirection, fragment.Normal);

    float minDistanceToRect = 1e9;
    int nearestRectIndex = -1;

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        float distanceToRect;
        vec2 uv;
        if (RayRectIntersect(reflectRay, lightRects[i], uv, distanceToRect))
        {
            if (distanceToRect < minDistanceToRect)
            {
                minDistanceToRect = distanceToRect;
                nearestRectIndex = i;
            }
        }
    }

    for (int i = 0; i < LIGHT_COUNT; i++)
    {            
        vec3 points[4];
        InitRectPoints(lightRects[i], points);

        vec3 lightColor = pow(uLights[i].Color_Height.rgb, vec3(GAMMA));
        vec3 specular = LTC_Evaluate(fragment.Normal, viewDirection, vPosition, Minv, points, uLightTextures[uLights[i].TextureIndex], uTextureSampler, twoSided);
        specularColor = specularColor * schlick.x + (1.0 - specularColor) * schlick.y;
        vec3 diffuse = LTC_Evaluate(fragment.Normal, viewDirection, vPosition, mat3(1), points, uLightTextures[uLights[i].TextureIndex], uTextureSampler, twoSided);
        vec3 color = lightColor * (specularColor * specular + diffuseColor * diffuse) / (2.0 * PI);

        if (nearestRectIndex != -1 && nearestRectIndex != i)
        {
            float blend = smoothstep(0.25, 0.45, fragment.Roughness);
            color *= pow(blend, 0.4);
        }
            
        totalColor += color;
    }
    
    minDistanceToRect = 1e9;
    nearestRectIndex = -1;
    vec2 intersectUV = vec2(0.0);
    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        float distanceToRect;
        vec2 uv;
        if (RayRectIntersect(ray, lightRects[i], uv, distanceToRect))
        {
            if (distanceToRect < minDistanceToRect && distanceToRect < length(vPosition - uCameraPosition))
            {
                minDistanceToRect = distanceToRect;
                nearestRectIndex = i;
                intersectUV = uv;
            }
        }
    }

    if (nearestRectIndex != -1)
    {
        LightData light = uLights[nearestRectIndex];
        vec3 lightColor = light.Color_Height.rgb * FetchDiffuseTexture(uLightTextures[nonuniformEXT(light.TextureIndex)], uTextureSampler, intersectUV);
        totalColor = pow(lightColor, vec3(GAMMA));
    }

    oColor = vec4(pow(totalColor, vec3(1.0 / GAMMA)), 1.0);
}