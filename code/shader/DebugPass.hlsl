#include "Constant.hlsli"
#include "StaticSamplers.hlsli"

DEFINE_RENDER_TARGET_SIZE_CONSTANT(b0)

cbuffer MapIndicesConstant : register(b1)
{
    uint mapIndices[10];
}

#define TEXTURE_ARRAY_SIZE 128
Texture2D _2DMaps[TEXTURE_ARRAY_SIZE] : register(t0, space0);
TextureCube _CubeMaps[TEXTURE_ARRAY_SIZE] : register(t0, space1);

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};
 
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD0;
};


VertexOut vs(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
    return vout;
}

float4 ps(VertexOut pin) : SV_TARGET
{
    return float4(_2DMaps[mapIndices[0]].Sample(_SamplerLinearClamp, pin.TexC).rrr, 1.0f);
}