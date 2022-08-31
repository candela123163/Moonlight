#include "Constant.hlsli"
#include "StaticSamplers.hlsli"

DEFINE_RENDER_TARGET_SIZE_CONSTANT(b0)

cbuffer MapIndicesConstant : register(b1)
{
    uint m0;
    uint m1;
    uint m2;
    uint m3;
    uint m4;
    uint m5;
    uint m6;
    uint m7;
    uint m8;
    uint m9;
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
#ifdef REVERSE_Z
    vout.PosH.z = 1.0f;
#endif
    return vout;
}


float4 ps(VertexOut pin) : SV_TARGET
{
    
    
    float2 uv = pin.TexC;
    
    if(uv.x < 0.5f && uv.y < 0.5f)
    {
        return _2DMaps[m0].Sample(_SamplerLinearClamp, uv).rrra;
    }
    else if(uv.x < 1.0f && uv.y < 0.5f)
    {
        return _2DMaps[m1].Sample(_SamplerLinearClamp, uv).rrra;
    }
    else if(uv.x < 0.5f && uv.y < 1.0f)
    {
        return _2DMaps[m2].Sample(_SamplerLinearClamp, uv).rrra;
    }
    else
    {
        return _2DMaps[m3].Sample(_SamplerLinearClamp, uv).rrra;
    }
    
  
    
    
    
    
}