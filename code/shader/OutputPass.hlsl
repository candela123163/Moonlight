#include "StaticSamplers.hlsli"

Texture2D _ColorMap : register(t0);


// Neutral tonemapping (Hable/Hejl/Frostbite)
// Input is linear RGB
float3 NeutralCurve(float3 x, float a, float b, float c, float d, float e, float f)
{
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float3 NeutralTonemap(float3 x)
{
    // Tonemap
    const float a = 0.2;
    const float b = 0.29;
    const float c = 0.24;
    const float d = 0.272;
    const float e = 0.02;
    const float f = 0.3;
    const float whiteLevel = 5.3;
    const float whiteClip = 1.0;

    float3 whiteScale = (1.0).xxx / NeutralCurve(whiteLevel, a, b, c, d, e, f);
    x = NeutralCurve(x * whiteScale, a, b, c, d, e, f);
    x *= whiteScale;

    // Post-curve white point adjustment
    x /= whiteClip.xxx;

    return x;
}

#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"


float4 ps(PostProc_VSOut pin) : SV_Target
{
    float4 color = _ColorMap.SampleLevel(_SamplerLinearClamp, pin.uv, 0);
    float3 toneMapped = NeutralTonemap(color.rgb);
    return float4(toneMapped, color.a);
}