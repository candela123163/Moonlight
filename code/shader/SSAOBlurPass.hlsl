#include "Util.hlsli"

Texture2D _2DMaps[] : register(t0, space0);
Texture2D<float> _AOMap : register(t0, space1);

cbuffer BlurConstant : register(b0)
{
    int _BlurRadius;
    float _RangeSigma;
    float _DepthSigma;
    float _NormalSigma;
    
    int _NormalMapIndex;
    int _DepthMapIndex;
    int _MapWidth;
    int _MapHeight;
}

RWTexture2D<float> _Output : register(u0);

#define GROUP_SIZE 256
#define MAX_BLUR_RADIUS 10
#define CACHE_SIZE (GROUP_SIZE + 2 * MAX_BLUR_RADIUS)

groupshared float _CacheAO[CACHE_SIZE];
groupshared float _CacheDepth[CACHE_SIZE];
groupshared float3 _CacheNormal[CACHE_SIZE];


void SetCache(int cacheIdx, int2 xy)
{
    _CacheAO[cacheIdx] = _AOMap[xy].r;
    _CacheDepth[cacheIdx] = _2DMaps[_DepthMapIndex][xy].r;
    _CacheNormal[cacheIdx] = _2DMaps[_NormalMapIndex][xy].xyz;
}


[numthreads(GROUP_SIZE, 1, 1)]
void XBlur_cs(
    int3 groupThreadID : SV_GroupThreadID, 
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    if(groupThreadID.x < _BlurRadius)
    {
        int x = max(0, dispatchThreadID.x - _BlurRadius);
        int2 xy = int2(x, dispatchThreadID.y);
        SetCache(groupThreadID.x, xy);
    }
    if(groupThreadID.x >= GROUP_SIZE - _BlurRadius)
    {
        int x = min(_MapWidth - 1, dispatchThreadID.x + _BlurRadius);
        int2 xy = int2(x, dispatchThreadID.y);
        SetCache(groupThreadID.x + 2 * _BlurRadius, xy);
    }
    
    int2 xy = min(dispatchThreadID.xy, int2(_MapWidth - 1, _MapHeight - 1));
    
    SetCache(groupThreadID.x + _BlurRadius, xy);
    
    GroupMemoryBarrierWithGroupSync();
    
    float bluredAO = 0.0f;
    float totalWeight = 0.0f;
    
    int center = groupThreadID.x + _BlurRadius;
    float centerDepth = _CacheDepth[center];
    float3 centerNormal = _CacheNormal[center];
    for (int i = -_BlurRadius; i <= _BlurRadius; i++)
    {
        int neighbor = center + i;
        float ao = _CacheAO[neighbor];
        float neighborDepth = _CacheDepth[neighbor];
        float3 neighborNormal = _CacheNormal[neighbor];
        
        float weight =
            GaussianFilter(Square(center - neighbor), _RangeSigma) *
            GaussianFilter(Square(centerDepth - neighborDepth), _DepthSigma) *
            GaussianFilter(acos(dot(centerNormal, neighborNormal)), _NormalSigma);
        
        bluredAO += ao * weight;
        totalWeight += weight;
    }

    _Output[dispatchThreadID.xy] = bluredAO / totalWeight;
}

[numthreads(1, GROUP_SIZE, 1)]
void YBlur_cs(
    int3 groupThreadID : SV_GroupThreadID,
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    if (groupThreadID.y < _BlurRadius)
    {
        int y = max(0, dispatchThreadID.y - _BlurRadius);
        int2 xy = int2(dispatchThreadID.x, y);
        SetCache(groupThreadID.y, xy);
    }
    if (groupThreadID.y >= GROUP_SIZE - _BlurRadius)
    {
        int y = min(_MapHeight - 1, dispatchThreadID.y + _BlurRadius);
        int2 xy = int2(dispatchThreadID.x, y);
        SetCache(groupThreadID.y + 2 * _BlurRadius, xy);
    }
    
    int2 xy = min(dispatchThreadID.xy, int2(_MapWidth - 1, _MapHeight - 1));
    SetCache(groupThreadID.y + _BlurRadius, xy);
    
    GroupMemoryBarrierWithGroupSync();
    
    float bluredAO = 0.0f;
    float totalWeight = 0.0f;
    
    int center = groupThreadID.y + _BlurRadius;
    float centerDepth = _CacheDepth[center];
    float3 centerNormal = _CacheNormal[center];
    for (int i = -_BlurRadius; i <= _BlurRadius; i++)
    {
        int neighbor = center + i;
        float ao = _CacheAO[neighbor];
        float neighborDepth = _CacheDepth[neighbor];
        float3 neighborNormal = _CacheNormal[neighbor];
        
        float weight =
            GaussianFilter(Square(center - neighbor), _RangeSigma) *
            GaussianFilter(Square(centerDepth - neighborDepth), _DepthSigma) *
            GaussianFilter(acos(dot(centerNormal, neighborNormal)), _NormalSigma);
        
        bluredAO += ao * weight;
        totalWeight += weight;
    }

    _Output[dispatchThreadID.xy] = bluredAO / totalWeight;
}