#include "Constant.hlsli"
#include "StaticSamplers.hlsli"

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


#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"

float4 ps(PostProc_VSOut pin) : SV_TARGET
{
    float2 uv = pin.uv;
    uint texIndex;
    
    if(uv.x <= 0.5f && uv.y <= 0.5f)
    {
        uv *= 2.0f;
        texIndex = m0;
    }
    else if(uv.x <= 1.0f && uv.y <= 0.5f)
    {
        uv.x = uv.x * 2.0f - 1.0f;
        uv.y *= 2.0f;
        texIndex = m1;
    }
    else if(uv.x <= 0.5f && uv.y <= 1.0f)
    {
        uv.x *= 2.0f;
        uv.y = uv.y * 2.0f - 1.0f;
        texIndex = m2;
    }
    else
    {
        uv.x = uv.x * 2.0f - 1.0f;
        uv.y = uv.y * 2.0f - 1.0f;
        texIndex = 3;    
    }
    
    return _2DMaps[texIndex].Sample(_SamplerLinearClamp, uv);
    
}