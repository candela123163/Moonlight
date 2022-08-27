#ifndef _UTIL_INCLUDE
#define _UTIL_INCLUDE

// =========================== math define =================================
#define PI            3.14159265359
#define TWO_PI        6.28318530718
#define FOUR_PI       12.56637061436
#define FOUR_PI2      39.47841760436
#define INV_PI        0.31830988618
#define INV_TWO_PI    0.15915494309
#define INV_FOUR_PI   0.07957747155
#define HALF_PI       1.57079632679
#define INV_HALF_PI   0.636619772367
// ==========================================================================

// =============================== misc =====================================
float Square(float x)
{
    return x * x;
}
// ==========================================================================


// =========================== space transform ==============================
float3 NormalTangentToWorld(float3 normalTS, float3 T, float3 B, float3 N)
{
    return normalize(mul(normalTS, float3x3(T, B, N)));
}

float3 NormalTangentToWorld(float3 normalTs, float3 N)
{
    float3 up = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    return NormalTangentToWorld(normalTs, T, B, N);
}
// ==========================================================================

// =========================== cubemap ====================================== 
float4 CubeMapVertexTransform(float3 posL, float4x4 viewProject, float3 eyePosW)
{
    float4 posW = float4(posL, 1.0f);
    posW.xyz += eyePosW;
    float4 posH = mul(posW, viewProject).xyww;
    return posH;
}

#define CUBEMAPFACE_POSITIVE_X 0
#define CUBEMAPFACE_NEGATIVE_X 1
#define CUBEMAPFACE_POSITIVE_Y 2
#define CUBEMAPFACE_NEGATIVE_Y 3
#define CUBEMAPFACE_POSITIVE_Z 4
#define CUBEMAPFACE_NEGATIVE_Z 5

uint CubeMapFaceID(float3 dir)
{
    uint faceID;

    if (abs(dir.z) >= abs(dir.x) && abs(dir.z) >= abs(dir.y))
    {
        faceID = (dir.z < 0.0) ? CUBEMAPFACE_NEGATIVE_Z : CUBEMAPFACE_POSITIVE_Z;
    }
    else if (abs(dir.y) >= abs(dir.x))
    {
        faceID = (dir.y < 0.0) ? CUBEMAPFACE_NEGATIVE_Y : CUBEMAPFACE_POSITIVE_Y;
    }
    else
    {
        faceID = (dir.x < 0.0) ? CUBEMAPFACE_NEGATIVE_X : CUBEMAPFACE_POSITIVE_X;
    }

    return faceID;
}

static const float3 CubeFaceNormals[6] =
{
    float3(1.0, 0.0, 0.0),
	float3(-1.0, 0.0, 0.0),
	float3(0.0, 1.0, 0.0),
	float3(0.0, -1.0, 0.0),
	float3(0.0, 0.0, 1.0),
	float3(0.0, 0.0, -1.0)
};

float3 GetCubeFaceNormal(float3 dir)
{
    return CubeFaceNormals[CubeMapFaceID(dir)];
}

// ==========================================================================

// =============================== sample =====================================
// low-discrepancy sequence
float _RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), _RadicalInverse_VdC(i));
}

float2 SampleUniformInDisk(float2 Xi)
{
    float theta = TWO_PI * Xi.y;
    float r = sqrt(Xi.x);
    return float2(r * cos(theta), r * sin(theta));
}

float2 SampleUniformCentricInDisk(float2 Xi)
{
    Xi = Xi * 2.0f - float2(1.0f, 1.0f);
    float theta, r;
    if (abs(Xi.x) > abs(Xi.y))
    {
        r = Xi.x;
        theta = INV_FOUR_PI * (Xi.y / Xi.x);
    }
    else
    {
        r = Xi.y;
        theta = INV_TWO_PI - INV_FOUR_PI * (Xi.x / Xi.y);
    }
    return r * float2(cos(theta), sin(theta));
}

float3 SampleUniformOnSphere(float2 Xi)
{
    float t = 2.0f * sqrt(Xi.x * (1.0f - Xi.x));
    float phi = TWO_PI * Xi.y;

    float x = cos(phi) * t;
    float y = sin(phi) * t;
    float z = 1.0f - 2.0f * Xi.x;

    return float3(x, y, z);
}

float3 SampleCosOnHemiSphere(float2 Xi)
{
    float2 pInDisk = SampleUniformInDisk(Xi);
    float z = sqrt(max(0.0f, 1.0f - Square(pInDisk.x) - Square(pInDisk.y)));
    return float3(pInDisk, z);
}
// ==========================================================================


#endif