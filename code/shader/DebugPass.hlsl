#include "Constant.hlsli"
#include "StaticSamplers.hlsli"
#include "Util.hlsli"

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


Texture2D _2DMaps[] : register(t0, space0);
TextureCube _CubeMaps[] : register(t0, space1);


#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"

float4 ps(PostProc_VSOut pin) : SV_TARGET
{
    float2 uv = pin.uv;
    
    if(uv.x <= 0.5f && uv.y <= 0.5f)
    {
        uv *= 2.0f;
        
        return float4(_2DMaps[27].SampleLevel(_SamplerLinearClamp, uv, 0).rrr, 1.0f);
    }
    else if(uv.x <= 1.0f && uv.y <= 0.5f)
    {
        uv.x = uv.x * 2.0f - 1.0f;
        uv.y *= 2.0f;
        
        return float4(_2DMaps[28].SampleLevel(_SamplerLinearClamp, uv, 0).rrr, 1.0f);
    }
    else if(uv.x <= 0.5f && uv.y <= 1.0f)
    {
        uv.x *= 2.0f;
        uv.y = uv.y * 2.0f - 1.0f;
        
        return float4(_2DMaps[37].SampleLevel(_SamplerLinearClamp, uv, 0).rgb, 1.0f);

    }
    else
    {
        uv.x = uv.x * 2.0f - 1.0f;
        uv.y = uv.y * 2.0f - 1.0f;
            
        return float4(_2DMaps[36].SampleLevel(_SamplerLinearClamp, uv, 0).rgb, 1.0f);

    }
    
    
}