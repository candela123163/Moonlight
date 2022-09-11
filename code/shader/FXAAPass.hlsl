#include "StaticSamplers.hlsli"

#define GROUP_SIZE 256

Texture2D<float4> _ColorMap : register(t0);
RWTexture2D<float4> _Output : register(u0);

cbuffer FXAAConstant : register(b0)
{
    float2 _TexelSize;
    float _FixedThreshold;
    float _RelativeThreshold;
    float _SubPixelBlending;
}

struct LumaNeighbor
{
    float m, n, e, s, w, ne, se, sw, nw;
    float highest, lowest, range;
};

struct Edge
{
    bool isHorizontal;
    float pixelStep;
    float lumaGradient;
    float peakLuma;
};

float GetLuma(float2 uv, float xOffset = 0.0f, float yOffset = 0.0f)
{
    uv += float2(xOffset, yOffset) * _TexelSize;
    return _ColorMap.SampleLevel(_SamplerLinearClamp, uv, 0).a;
}

LumaNeighbor GetNeighbor(float2 uv)
{
    LumaNeighbor neighbor;
    neighbor.m = GetLuma(uv);
    neighbor.n = GetLuma(uv, 0.0, 1.0);
    neighbor.e = GetLuma(uv, 1.0, 0.0);
    neighbor.s = GetLuma(uv, 0.0, -1.0);
    neighbor.w = GetLuma(uv, -1.0, 0.0);
    neighbor.ne = GetLuma(uv, 1.0, 1.0);
    neighbor.se = GetLuma(uv, 1.0, -1.0);
    neighbor.sw = GetLuma(uv, -1.0, -1.0);
    neighbor.nw = GetLuma(uv, -1.0, 1.0);

    neighbor.highest = max(max(max(max(neighbor.m, neighbor.n), neighbor.e), neighbor.s), neighbor.w);
    neighbor.lowest = min(min(min(min(neighbor.m, neighbor.n), neighbor.e), neighbor.s), neighbor.w);
    neighbor.range = neighbor.highest - neighbor.lowest;
    return neighbor;
}

bool IsHorizontalEdge(LumaNeighbor neighbor)
{
    float horizontal =
		2.0 * abs(neighbor.n + neighbor.s - 2.0 * neighbor.m) +
		abs(neighbor.ne + neighbor.se - 2.0 * neighbor.e) +
		abs(neighbor.nw + neighbor.sw - 2.0 * neighbor.w);
    float vertical =
		2.0 * abs(neighbor.e + neighbor.w - 2.0 * neighbor.m) +
		abs(neighbor.ne + neighbor.nw - 2.0 * neighbor.n) +
		abs(neighbor.se + neighbor.sw - 2.0 * neighbor.s);
    return horizontal >= vertical;
}

Edge GetEdge(LumaNeighbor neighbor)
{
    Edge edge;
    edge.isHorizontal = IsHorizontalEdge(neighbor);
    float lumaPositive, lumaNegative;
    if(edge.isHorizontal)
    {
        edge.pixelStep = _TexelSize.y;
        lumaPositive = neighbor.n;
        lumaNegative = neighbor.s;
    }
    else
    {
        edge.pixelStep = _TexelSize.x;
        lumaPositive = neighbor.e;
        lumaNegative = neighbor.w;
    }
    
    float gradientPositive = abs(lumaPositive - neighbor.m);
    float gradientNegative = abs(lumaNegative - neighbor.m);
    
    if(gradientPositive < gradientNegative)
    {
        edge.pixelStep = -edge.pixelStep;
        edge.lumaGradient = gradientNegative;
        edge.peakLuma = lumaNegative;
    }
    else
    {
        edge.lumaGradient = gradientPositive;
        edge.peakLuma = lumaPositive;
    }
    
    return edge;
}

bool CanSkipFXAA(LumaNeighbor neighbor)
{
    return neighbor.range < max(_FixedThreshold, _RelativeThreshold * neighbor.highest);
}

float GetSubPixelBlending(LumaNeighbor neighbor)
{
    float filter = 2.0 * (neighbor.n + neighbor.e + neighbor.s + neighbor.w);
    filter += neighbor.ne + neighbor.nw + neighbor.se + neighbor.sw;
    filter *= 1.0 / 12.0;
    filter = abs(filter - neighbor.m);
    filter = saturate(filter / neighbor.range);
    filter = smoothstep(0, 1, filter);
    return filter * filter * _SubPixelBlending;
}

#define FXAA_QUALITY_MEDIUM

#if defined(FXAA_QUALITY_LOW)
    #define EXTRA_EDGE_STEPS 3
    #define EDGE_STEP_SIZES 1.5, 2.0, 2.0
    #define LAST_EDGE_STEP_GUESS 8.0
#elif defined(FXAA_QUALITY_MEDIUM)
    #define EXTRA_EDGE_STEPS 8
    #define EDGE_STEP_SIZES 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0
    #define LAST_EDGE_STEP_GUESS 8.0
#else
    #define EXTRA_EDGE_STEPS 10
    #define EDGE_STEP_SIZES 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0
    #define LAST_EDGE_STEP_GUESS 8.0
#endif
static const float edgeStepSizes[EXTRA_EDGE_STEPS] = { EDGE_STEP_SIZES };


float GetEdgeBlending(LumaNeighbor neighbor, Edge edge, float2 uv)
{
    float2 edgeUV = uv;
    float2 uvStep = 0.0f;
    if(edge.isHorizontal)
    {
        edgeUV.y += 0.5f * edge.pixelStep;
        uvStep.x = _TexelSize.x;
    }
    else
    {
        edgeUV.x += 0.5f * edge.pixelStep;
        uvStep.y = _TexelSize.y;
    }
    
    float edgeLuma = 0.5f * (neighbor.m + edge.peakLuma);
    float edgeThreshold = 0.25f * edge.lumaGradient;
    
    // try find edge end point on positive direction
    float2 uvP = edgeUV + uvStep;
    float lumaDeltaP = GetLuma(uvP) - edgeLuma;
    bool atEndP = abs(lumaDeltaP) >= edgeThreshold;
    
    [unroll]
    for (int i = 0; i < EXTRA_EDGE_STEPS && !atEndP; i++)
    {
        uvP += uvStep * edgeStepSizes[i];
        lumaDeltaP = GetLuma(uvP) - edgeLuma;
        atEndP = abs(lumaDeltaP) >= edgeThreshold;
    }
    if(!atEndP)
    {
        uvP += uvStep * LAST_EDGE_STEP_GUESS;
    }
    
    // try find edge end point on negative direction
    float2 uvN = edgeUV - uvStep;
    float lumaDeltaN = GetLuma(uvN) - edgeLuma;
    bool atEndN = abs(lumaDeltaN) >= edgeThreshold;
    
    [unroll]
    for (int j = 0; j < EXTRA_EDGE_STEPS && !atEndN; j++)
    {
        uvN -= uvStep * edgeStepSizes[j];
        float lumaDeltaN = GetLuma(uvN) - edgeLuma;
        bool atEndN = abs(lumaDeltaN) >= edgeThreshold;
    }
    if(!atEndN)
    {
        uvN -= uvStep * LAST_EDGE_STEP_GUESS;
    }
    
    float distanceToEndP, distanceToEndN;
    if(edge.isHorizontal)
    {
        distanceToEndP = uvP.x - uv.x;
        distanceToEndN = uv.x - uvN.x;
    }
    else
    {
        distanceToEndP = uvP.y - uv.y;
        distanceToEndN = uv.y - uvN.y;
    }
    
    float distanceToNearestEnd;
    int signDelta;
    if(distanceToEndP < distanceToEndN)
    {
        distanceToNearestEnd = distanceToEndP;
        signDelta = sign(lumaDeltaP);
    }
    else
    {
        distanceToNearestEnd = distanceToEndN;
        signDelta = sign(lumaDeltaN);
    }

    if (signDelta == sign(neighbor.m - edge.peakLuma))
    {
        return 0.0f;
    }
    else
    {
        return 0.5f - distanceToNearestEnd / (distanceToEndN + distanceToEndP);
    }
}


[numthreads(GROUP_SIZE, 1, 1)]
void FXAA_cs(int3 dispatchThreadID : SV_DispatchThreadID)
{
    
    int2 xy = dispatchThreadID.xy;
    float2 uv = xy * _TexelSize;
    
    LumaNeighbor neighbor = GetNeighbor(uv);
    
    if (!CanSkipFXAA(neighbor))
    {
        Edge edge = GetEdge(neighbor);
        float blending = max(
            GetSubPixelBlending(neighbor),
            GetEdgeBlending(neighbor, edge, uv)
        );
                
        if (edge.isHorizontal)
        {
            uv.y += blending * edge.pixelStep;
        }
        else
        {
            uv.x += blending * edge.pixelStep;
        }
    }

    _Output[xy] = _ColorMap.SampleLevel(_SamplerLinearClamp, uv, 0);
    

}