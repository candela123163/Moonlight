#include "StaticSamplers.hlsli"
#include "Constant.hlsli"

DEFINE_CAMERA_CONSTANT(b0)

cbuffer TAAConstant : register(b1)
{
    uint _FrameColorMapIndex;
    uint _FrameDepthMapIndex;
    uint _HistoryColorMapIndex;
    uint _MotionMapIndex;
    
    float2 _TexelSize;
}

Texture2D _2DMaps[] : register(t0);


#define FullScreenTriangle_VS vs
#include "FullScreenVS.hlsli"

#define USE_YCOCG
#define TONE_BOUND 0.5
#define EXPOSURE 10.0

#define HISTORY_CLIP_LEFT 1.25
#define HISTORY_CLIP_RIGHT 6.0
#define HISTORY_RESOLVE_LEFT  0.05
#define HISTORY_RESOLVE_RIGHT 0.15
#define MOTION_WEIGHT_FACTOR 1000

#define SHARPNESS 0.1

float Luma(float3 color)
{
    return (color.g * 0.5) + (color.r + color.b) * 0.25;
}

float Luma4(float3 color)
{
    return (color.g * 2) + (color.r + color.b);
}

float3 Tonemap(float3 color)
{
    float luma = Luma(color);
    [flatten]
    if (luma <= TONE_BOUND)
        return color;
    else
        return color * (TONE_BOUND * TONE_BOUND - luma) / (luma * (2 * TONE_BOUND - 1 - luma));
}

float3 TonemapInvert(float3 x)
{
    float luma = Luma(x);
    [flatten]
    if (luma <= TONE_BOUND)
        return x;
    else
        return x * (TONE_BOUND * TONE_BOUND - (2 * TONE_BOUND - 1) * luma) / (luma * (1 - luma));
}

float3 RGBToYCoCg(float3 RGB)
{
#ifdef USE_YCOCG
        const float3x3 YCOCGMatrix = float3x3(
        1, 2,  1,
        2, 0, -2,
        -1, 2, -1
        );
        /*float Y  = dot( RGB, float3(  1, 2,  1 ) );
        float Co = dot( RGB, float3(  2, 0, -2 ) );
        float Cg = dot( RGB, float3( -1, 2, -1 ) );*/
        
        float3 YCoCg = mul(YCOCGMatrix, RGB);
        return YCoCg;
#else
    return RGB;
#endif
}

float3 YCoCgToRGB(float3 YCoCg)
{
#ifdef USE_YCOCG
        YCoCg  = YCoCg * 0.25;

        float R = YCoCg.x + YCoCg.y - YCoCg.z;
        float G = YCoCg.x + YCoCg.z;
        float B = YCoCg.x - YCoCg.y - YCoCg.z;

        float3 RGB = float3( R, G, B );
        return RGB;
#else
    return YCoCg;
#endif
}

float3 ClipToAABB(float3 color, float3 minimum, float3 maximum)
{
    // Note: only clips towards aabb center (but fast!)
    float3 center = 0.5 * (maximum + minimum);
    float3 extents = 0.5 * (maximum - minimum);

    // This is actually `distance`, however the keyword is reserved
    float3 offset = color.rgb - center;

    float3 ts = abs(extents / (offset + 0.0001));
    float t = saturate(min(min(ts.x, ts.y), ts.z));
    color.rgb = center + offset * t;
    return color;
}

float HdrWeight4(float3 Color, const float Exposure)
{
#ifdef USE_YCOCG
        return rcp(Color.r * Exposure + 4.0);
#else
    return rcp(Luma4(Color) * Exposure + 4);
#endif
}

static const int2 _NeighborOffset[9] =
{
    int2(-1, -1),
    int2(0, -1),
    int2(1, -1),
    
    int2(-1, 0),
    int2(0, 0),
    int2(1, 0),
    
    int2(-1, 1),
    int2(0, 1),
    int2(1, 1)
};

#ifdef REVERSE_Z
    #define DEPTH_CLOSER(reference, x) step(reference, x)
#else
    #define DEPTH_CLOSER(reference, x) step(x, reference)
#endif

float2 ReprojectColsestMotion(float2 uv)
{
#ifdef REVERSE_Z
    float3 closest = float3(0.0f, 0.0f, 0.0f);
#else
    float3 closest = float3(0.0f, 0.0f, 1.0f);
#endif
    
    [unroll]
    for (int i = 0; i < 9; i++)
    {
        float depth = _2DMaps[_FrameDepthMapIndex].SampleLevel(_SamplerPointClamp, uv, 0, _NeighborOffset[i]).x;
        closest = lerp(closest, float3(_NeighborOffset[i], depth), DEPTH_CLOSER(closest.z, depth));
    }
    
    float2 closestUV = uv + closest.xy * _TexelSize;
    return _2DMaps[_MotionMapIndex].SampleLevel(_SamplerPointClamp, closestUV, 0).xy;
}


float4 ps(PostProc_VSOut pin) : SV_Target
{
    // reprojection   
    float2 jitteredUV = pin.uv + _Jitter;
    
    float4 curColor = _2DMaps[_FrameColorMapIndex].SampleLevel(_SamplerLinearClamp, jitteredUV, 0).rgba;
    float2 motion = ReprojectColsestMotion(pin.uv);
    
    float motionWeight = saturate(length(motion) * MOTION_WEIGHT_FACTOR);
    
    float2 preUV = pin.uv - motion;
    
    if (any(preUV < 0.0) || any(preUV > 1.0))
    {
        return curColor;
    }
        
    // rectify history
    float hdrWeights[9];
    float3 neighbors[9];
    float3 m1 = 0.0f;
    float3 m2 = 0.0f;
    float3 accumulatedColor = 0.0f;
    float totalWeight = 0.0f;
    
    [unroll]
    for (uint i = 0; i < 9; i++)
    {
        neighbors[i] = _2DMaps[_FrameColorMapIndex].SampleLevel(_SamplerLinearClamp, jitteredUV, 0, _NeighborOffset[i]).rgb;
        float3 tonemapped = Tonemap(neighbors[i]);
        float3 ycocgColor = RGBToYCoCg(tonemapped);
        hdrWeights[i] = HdrWeight4(ycocgColor, EXPOSURE);
        
        m1 += ycocgColor;
        m2 += ycocgColor * ycocgColor;
        accumulatedColor += ycocgColor * hdrWeights[i];
        totalWeight += hdrWeights[i];
    }
    
    float3 filteredColor = accumulatedColor / totalWeight;
    
    float3 mean = m1 / 9.0f;
    float3 stddev = sqrt(max(0.0f, m2 / 9.0f - mean * mean));
    float extend = lerp(HISTORY_CLIP_LEFT, HISTORY_CLIP_RIGHT, 1.0f - motionWeight);
    float3 minColor = mean - extend * stddev;
    float3 maxColor = mean + extend * stddev;
    minColor = min(minColor, filteredColor);
    maxColor = max(maxColor, filteredColor);
    
    float4 preColor = _2DMaps[_HistoryColorMapIndex].Sample(_SamplerLinearClamp, preUV).rgba;
    preColor.rgb = Tonemap(preColor.rgb);
    preColor.rgb = RGBToYCoCg(preColor.rgb);
    preColor.rgb = ClipToAABB(preColor.rgb, minColor, maxColor);
    
    
    // history resolve
    preColor.rgb = YCoCgToRGB(preColor.rgb);
    
    // sharpen
    float3 highFrequence = 0.25f * (neighbors[0] + neighbors[2] + neighbors[6] + neighbors[8]) +
                            0.5f * (neighbors[1] + neighbors[3] + neighbors[5] + neighbors[7]) -
                            3.0f * neighbors[4];
    curColor.rgb += highFrequence * SHARPNESS;
    
    curColor.rgb = Tonemap(curColor.rgb);
    float resolveFactor = lerp(HISTORY_RESOLVE_LEFT, HISTORY_RESOLVE_RIGHT, motionWeight);
    float4 blendedColor = lerp(preColor, curColor, resolveFactor);
    blendedColor.rgb = TonemapInvert(blendedColor.rgb);
    
    
    return blendedColor;
}