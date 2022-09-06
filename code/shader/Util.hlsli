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
#define EPSILON       0.01
// ==========================================================================

// =============================== misc =====================================
float Square(float x)
{
    return x * x;
}

float2 NDCXYToUV(float2 xy)
{
    float2 uv = xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return uv;
}

float FadeOut(float x, float upperBound, float fadeRange)
{
    fadeRange = saturate(fadeRange);
    return saturate((1.0f - x / upperBound) / fadeRange);
}
// ==========================================================================


// =========================== space transform ==============================
float3 NormalTangentToWorld(float3 normalTS, float3 T, float3 B, float3 N)
{
    return mul(normalTS, float3x3(T, B, N));
}

/*
       Z
       |
       |
       |
       O--------X
      /
     /
    /
   Y
*/
float3 NormalTangentToWorld(float3 normalTs, float3 N)
{
    float3 up = abs(N.z) < 1.0f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    return NormalTangentToWorld(normalTs, T, B, N);
}

float NDCDepthToViewDepth(float ndcDepth, float near, float far)
{
#ifdef REVERSE_Z
    float temp = far;
    far = near;
    near = temp;
#endif
    return near * far / (far - ndcDepth * (far - near));
}

float4 NDCToViewPosition(float2 screenCoord, float ndcDepth, float near, float far, float4x4 invProject)
{
    float viewDepth = NDCDepthToViewDepth(ndcDepth, near, far);
    screenCoord.xy = screenCoord.xy * 2.0f - 1.0f;
    screenCoord.y = -screenCoord.y;
    float4 posH = float4(screenCoord, ndcDepth, 1.0f) * viewDepth;
    return mul(posH, invProject);
}

float NDCDepthToLinear01Depth(float ndcDepth, float near, float far)
{
    return NDCDepthToViewDepth(ndcDepth, near, far) / far;
}
// ==========================================================================

// =========================== cubemap ====================================== 
float4 CubeMapVertexTransform(float3 posL, float4x4 viewProject, float3 eyePosW)
{
    float4 posW = float4(posL, 1.0f);
    posW.xyz += eyePosW;
    float4 posH = mul(posW, viewProject).xyww;
#ifdef REVERSE_Z
    posH.z = 0.0f;
#endif
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


static const float2 OffsetSelecter[6][3] =
{
    { float2(0.0, 0.0), float2(0.0, 1.0), float2(-1.0, 0.0) },  // +X
    { float2(0.0, 0.0), float2(0.0, 1.0), float2(1.0, 0.0) },  // -X
    { float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 0.0) },  // +Y
    { float2(0.0, 1.0), float2(0.0, 0.0), float2(-1.0, 0.0) },  // -Y
    { float2(1.0, 0.0), float2(0.0, 1.0), float2(0.0, 0.0) },  // +Z
    { float2(-1.0, 0.0), float2(0.0, 1.0), float2(0.0, 0.0) }   // -Z
};

// given a cube sample direction and a 2d offset, 
// calculate a new cube sample direction perturbed by this offset
// @param:
//  dir:  a cube sample direction
//  offset: 2d perturbe vector
// @return: a new cube sample direction
float3 GetCubeShadowSamplingDirection(float3 dir, float2 offset)
{
    float3 perturbed = dir / max(abs(dir.x), max(abs(dir.y), abs(dir.z)));
    uint faceID = CubeMapFaceID(dir);
    perturbed.x += dot(offset, OffsetSelecter[faceID][0]);
    perturbed.y += dot(offset, OffsetSelecter[faceID][1]);
    perturbed.z += dot(offset, OffsetSelecter[faceID][2]);
    return normalize(perturbed);
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

float _PRNG(float2 seed)
{
    return frac(sin(dot(seed, float2(12.9898, 78.233))) * 43758.5453);
}

static float _rand_last = 0.0f;
static float2 _uv = 0.0f;

void RandInit(float2 uv)
{
    _uv = uv;
}

float RandGetNext1()
{
    _rand_last = _PRNG(_uv + float2(_rand_last, 2.0f * _rand_last));
    return _rand_last;
}

float2 RandGetNext2()
{
    float2 rand;
    rand.x = RandGetNext1();
    rand.y = RandGetNext1();
    return rand;
}

float3 RandGetNext3()
{
    float3 rand;
    rand.x = RandGetNext1();
    rand.y = RandGetNext1();
    rand.z = RandGetNext1();
    return rand;
}

float4 RandGetNext4()
{
    float4 rand;
    rand.x = RandGetNext1();
    rand.y = RandGetNext1();
    rand.z = RandGetNext1();
    rand.w = RandGetNext1();
    return rand;
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

float3 SampleUniformOnHemiSphere(float2 Xi)
{
    float z = Xi.x;
    float sinTheta = sqrt(1.0f - Xi.x * Xi.x);
    float phi = TWO_PI * Xi.y;
    
    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    
    return float3(x, y, z);
}

float3 SampleUniformInsideHemiSphere(float3 Xi)
{
    float3 v = SampleUniformOnHemiSphere(Xi.xy);
    float r = pow(Xi.z, 1.0f / 3.0f);
    return r * v;
}

float3 SampleCosOnHemiSphere(float2 Xi)
{
    float2 pInDisk = SampleUniformInDisk(Xi);
    float z = sqrt(max(0.0f, 1.0f - Square(pInDisk.x) - Square(pInDisk.y)));
    return float3(pInDisk, z);
}

float3 SampleCosInsideHemiSphere(float3 Xi)
{
    float3 v = SampleCosOnHemiSphere(Xi.xy);
    float r = pow(Xi.z, 1.0f / 3.0f);
    return r * v;
}
// ==========================================================================

// ================================== Filter ================================
float GaussianFilter(float difference, float sigma)
{
    return exp(-difference / (2.0f * sigma * sigma));
}

// ==========================================================================
#endif