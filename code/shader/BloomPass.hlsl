#include "StaticSamplers.hlsli"

#define GROUP_SIZE 256

#define BLOOM_CLAMP  65000.f

Texture2D<float4> _BloomChain : register(t0);
Texture2D<float4> _InputMap : register(t1);
RWTexture2D<float4> _Output : register(u0);

cbuffer BloomConstant : register(b0)
{
    float4 _Threshold;
    
    float _Intensity;
}

cbuffer MipLevelConstant : register(b1)
{   
    uint _BloomChainMipLevel;
    uint _InputMapMipLevel;
}


float2 GetOutputUV(int2 xy)
{
    float w, h;
    _Output.GetDimensions(w, h);
    return xy / float2(w, h);
}


// SIGGRAPH 2014
// Better, temporally stable box filtering
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
float4 DownsampleBox13Tap(Texture2D<float4> tex, uint mipLevel, float2 uv)
{
    float w, h, _;
    tex.GetDimensions(mipLevel, w, h, _);
    float2 texelSize = float2(1.0f / w, 1.0f / h);
    
    float4 A = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(-1.0, -1.0), mipLevel);
    float4 B = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(0.0, -1.0), mipLevel);
    float4 C = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(1.0, -1.0), mipLevel);
    float4 D = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(-0.5, -0.5), mipLevel);
    float4 E = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(0.5, -0.5), mipLevel);
    float4 F = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(-1.0, 0.0), mipLevel);
    float4 G = tex.SampleLevel(_SamplerLinearClamp, uv, mipLevel);
    float4 H = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(1.0, 0.0), mipLevel);
    float4 I = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(-0.5, 0.5), mipLevel);
    float4 J = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(0.5, 0.5), mipLevel);
    float4 K = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(-1.0, 1.0), mipLevel);
    float4 L = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(0.0, 1.0), mipLevel);
    float4 M = tex.SampleLevel(_SamplerLinearClamp, uv + texelSize * float2(1.0, 1.0), mipLevel);

    float2 div = (1.0 / 4.0) * float2(0.5, 0.125);

    float4 o = (D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    return o;
}

float4 UpsampleTent(Texture2D tex, uint mipLevel, float2 uv)
{
    float w, h, _;
    tex.GetDimensions(mipLevel, w, h, _);
    float2 texelSize = float2(1.0f / w, 1.0f / h);
    float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0);

    float4 s;
    s = tex.SampleLevel(_SamplerLinearClamp, uv - d.xy, mipLevel);
    s += tex.SampleLevel(_SamplerLinearClamp, uv - d.wy, mipLevel) * 2.0;
    s += tex.SampleLevel(_SamplerLinearClamp, uv - d.zy, mipLevel);

    s += tex.SampleLevel(_SamplerLinearClamp, uv + d.zw, mipLevel) * 2.0;
    s += tex.SampleLevel(_SamplerLinearClamp, uv, mipLevel) * 4.0;
    s += tex.SampleLevel(_SamplerLinearClamp, uv + d.xw, mipLevel) * 2.0;

    s += tex.SampleLevel(_SamplerLinearClamp, uv + d.zy, mipLevel);
    s += tex.SampleLevel(_SamplerLinearClamp, uv + d.wy, mipLevel) * 2.0;
    s += tex.SampleLevel(_SamplerLinearClamp, uv + d.xy, mipLevel);

    return s * (1.0 / 16.0);
}

float4 QuadraticThreshold(float4 color, float threshold, float3 curve)
{
	// Pixel brightness
    float br = max(max(color.r, color.g), color.b);
    
	// Under-threshold part: quadratic curve
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

	// Combine and apply the brightness response curve.
    color *= max(rq, br - threshold) / (br + 0.0001f);

    return color;
}

float Luminance(float3 linearRgb)
{
    return dot(linearRgb, float3(0.2126729, 0.7151522, 0.0721750));
}

float4 Prefilter(float4 color)
{
    color = min(color, BLOOM_CLAMP);
    // x: threshold value (linear), y: threshold - knee, z: knee * 2, w: 0.25 / knee
    color = QuadraticThreshold(color, _Threshold.x, _Threshold.yzw);
    float luminance = Luminance(color.rgb);
    return color / (luminance + 1.0f);
}

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


[numthreads(GROUP_SIZE, 1, 1)]
void Prefiler_cs(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 xy = dispatchThreadID.xy;
    float2 uv = GetOutputUV(dispatchThreadID.xy);
    
    float4 color = DownsampleBox13Tap(_InputMap, _InputMapMipLevel, uv);
    _Output[xy] = Prefilter(color);
}

[numthreads(GROUP_SIZE, 1, 1)]
void DownSample_cs(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 xy = dispatchThreadID.xy;
    float2 uv = GetOutputUV(dispatchThreadID.xy);
    
    float4 color = DownsampleBox13Tap(_BloomChain, _BloomChainMipLevel, uv);
    _Output[xy] = color;
}

[numthreads(GROUP_SIZE, 1, 1)]
void UpSample_cs(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 xy = dispatchThreadID.xy;
    float2 uv = GetOutputUV(dispatchThreadID.xy);
    
    float4 accumulatedBloom = UpsampleTent(_BloomChain, _BloomChainMipLevel, uv);
    float4 bloom = _InputMap.SampleLevel(_SamplerLinearClamp, uv, _InputMapMipLevel);
    _Output[xy] = accumulatedBloom + _Intensity * bloom;
}

[numthreads(GROUP_SIZE, 1, 1)]
void Combine_cs(int3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 xy = dispatchThreadID.xy;
    float2 uv = GetOutputUV(dispatchThreadID.xy);
    
    float4 bloom = UpsampleTent(_BloomChain, _BloomChainMipLevel, uv);
    float4 color = _InputMap.SampleLevel(_SamplerLinearClamp, uv, _InputMapMipLevel);
    float3 toneMapped = NeutralTonemap(color.rgb + bloom.rgb * _Intensity);
    float3 gammaCorrect = pow(toneMapped, 1.0f / 2.2f);
    float gammaSpaceLuma = sqrt(Luminance(toneMapped));
    _Output[xy] = float4(gammaCorrect, gammaSpaceLuma);
}