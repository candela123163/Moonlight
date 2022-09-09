#include "StaticSamplers.hlsli"

Texture2D _ColorMap : register(t0);

#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"

float4 ps(PostProc_VSOut pin) : SV_Target
{
    float4 color = _ColorMap.SampleLevel(_SamplerLinearClamp, pin.uv, 0);

    return color;
}