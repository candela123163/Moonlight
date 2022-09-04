#include "StaticSamplers.hlsli"
#include "Constant.hlsli"
#include "Util.hlsli"

Texture2D<float> _DepthMap : register(t0);
Texture2D<float4> _NormalMap : register(t1);

DEFINE_RENDER_TARGET_PARAM_CONSTANT(b0)
DEFINE_CAMERA_CONSTANT(b1)

cbuffer HBAOConsant : register(b2)
{
    float _FocalLength;
    float _R;
    float _R2;
    float _NegInvR2;
    
    float _TangentBias;
    uint _NumDirections;
    uint _NumSamples;
    float _AOPower;
}

#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"

float3 GetViewPos(float2 uv)
{
    float ndcDepth = _DepthMap.SampleLevel(_SamplerPointClamp, uv, 0);
    return NDCToViewPosition(uv, ndcDepth, _NearFar.x, _NearFar.y, _InvProj).xyz;
}

float ps(PostProc_VSOut pin) : SV_Target
{
    

}