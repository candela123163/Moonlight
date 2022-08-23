#include "Constant.hlsli"
#include "Util.hlsli"
#include "StaticSamplers.hlsli"

DEFINE_CAMERA_CONSTANT(b0)
TextureCube _EnvMap : register(t0);

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posL : VAR_POSITION_L;
};


VertexOut vs(float3 posL : POSITION) 
{
    VertexOut vout;
    vout.posH = CubeMapVertexTransform(posL, _ViewProj, _EyePosW.xyz);
    vout.posL = posL;
    return vout;
}

float4 ps(VertexOut pin) : SV_TARGET
{
    float3 color = _EnvMap.Sample(_SamplerLinearWrap, pin.posL).rgb;
    return float4(color, 1.0f);
}