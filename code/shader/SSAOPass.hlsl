#include "StaticSamplers.hlsli"
#include "Constant.hlsli"
#include "Util.hlsli"

Texture2D _2DMaps[] : register(t0, space0);

DEFINE_RENDER_TARGET_PARAM_CONSTANT(b0)
DEFINE_CAMERA_CONSTANT(b1)

cbuffer SSAOConsant : register(b2)
{
    float _AORadius;
    uint _AOSampleCount;
    float _AOPower;
    float _AOFadeRange;
    
    uint _NormalMapIndex;
    uint _DepthMapIndex;
}

#define INV_PROJECT _InvProj
#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"


float3 GetNormal(float2 uv)
{
    return _2DMaps[_NormalMapIndex].SampleLevel(_SamplerPointClamp, uv, 0).xyz;
}

float GetNDCDepth(float2 uv)
{
    return _2DMaps[_DepthMapIndex].SampleLevel(_SamplerPointClamp, uv, 0).x;
}

float GetViewDepth(float2 uv)
{
    float ndcDepth = GetNDCDepth(uv);
    return NDCDepthToViewDepth(ndcDepth, _NearFar.x, _NearFar.y);
}

float3 GetViewPosition(float viewDepth, float3 ray)
{
    return ray * (viewDepth / ray.z);
}

float GetOcclusion(float3 pV, float3 nV, float3 sample_pV, float3 occluder_pV)
{
    float occlude = step(occluder_pV.z + EPSILON, clamp(sample_pV.z, _NearFar.x, _NearFar.y));
    float fade = FadeOut(distance(occluder_pV, pV), _AORadius, _AOFadeRange);
    return occlude * fade;
}

float ps(PostProc_VSOut pin) : SV_Target
{
    float3 nV = GetNormal(pin.uv);
    float zV = GetViewDepth(pin.uv);
    float3 pV = GetViewPosition(zV, pin.ray);

    RandInit(pin.uv);
    float occlusion = 0.0f;
    
    [loop]
    for (uint i = 0; i < _AOSampleCount; i++)
    {
        float3 rand = float3(Hammersley(i, _AOSampleCount), RandGetNext1());
        float3 offsetT = SampleCosInsideHemiSphere(rand);
        float3 offsetV = NormalTangentToWorld(offsetT, nV) * _AORadius;
  
        float3 sample_pV = pV + offsetV;
        float4 sample_pH = mul(float4(sample_pV, 1.0f), _Proj);
        sample_pH /= sample_pH.w;
        float2 sample_uv = NDCXYToUV(sample_pH.xy);
        
        float occluder_zV = GetViewDepth(sample_uv);
        float3 occluder_pV = GetViewPosition(occluder_zV, sample_pV);
        
        // SampleCosInsideHemiSphere
        // pdf = 3 * cos(theta) / PI
        // sample_occlusion = GetOcclusion(...) * cos(theta) / pdf = PI * GetOcclusion(...) / 3
        occlusion += PI * GetOcclusion(pV, nV, sample_pV, occluder_pV) / 3.0f;
    }
    
    float access = saturate(1.0f - occlusion / _AOSampleCount);
    return pow(access, _AOPower);
}