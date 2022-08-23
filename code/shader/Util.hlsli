#ifndef _UTIL_INCLUDE
#define _UTIL_INCLUDE

#define PI            3.14159265359
#define TWO_PI        6.28318530718
#define FOUR_PI       12.56637061436
#define FOUR_PI2      39.47841760436
#define INV_PI        0.31830988618
#define INV_TWO_PI    0.15915494309
#define INV_FOUR_PI   0.07957747155
#define HALF_PI       1.57079632679
#define INV_HALF_PI   0.636619772367

float Square(float x)
{
    return x * x;
}

float3 NormalTangentToWorld(float3 normalTS, float3 T, float3 B, float3 N)
{
    return mul(normalTS, float3x3(T, B, N));
}

float4 CubeMapVertexTransform(float3 posL, float4x4 viewProject, float3 eyePosW)
{
    float4 posW = float4(posL, 1.0f);
    posW.xyz += eyePosW;
    float4 posH = mul(posW, viewProject).xyww;
    return posH;
}

#endif