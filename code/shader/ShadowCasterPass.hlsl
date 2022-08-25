#include "StaticSamplers.hlsli"
#include "Constant.hlsli"

DEFINE_OBJ_CONSTANT(b0)
DEFINE_CAMERA_CONSTANT(b1)

Texture2D _Albedo : register(t0);

struct VertexIn
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float2 uv : VAR_TEXCOORD;
};

VertexOut vs(VertexIn vin)
{
    VertexOut vout;
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    vout.posH = mul(posW, _ViewProj);
    vout.uv = vin.uv;
    
    return vout;
}

void ps(VertexOut pin)
{
    float alpha = _Albedo.Sample(_SamplerLinearWrap, pin.uv).a;
    clip(alpha - 0.5f);
}