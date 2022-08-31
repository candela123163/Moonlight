#ifndef _SHADOW_INCLUDE
#define _SHADOW_INCLUDE

#include "LitInput.hlsli"
#include "LitDefine.hlsli"
#include "ShadowSamplingTent.hlsli"
#include "Util.hlsli"

#define FILTER_SAMPLER_SIZE 16
#define FILTER_SETUP SampleShadow_ComputeSamples_Tent_7x7

float ShadowFade(float depth, float maxDepth, float factor)
{
    return saturate((maxDepth / depth - 1.0f) * factor);
}

float GetShadowGlobalFade(float depth)
{
    return ShadowFade(depth, _ShadowMaxDistance, 2.0f);
}


// Spot Shadow
float GetSpotShadowAttenuation(Surface surface, uint index)
{
    float attenuation = 1.0f;
    SpotLight light = _SpotLights[index];
    SpotShadow shadow = _ShadowSpot[index];
    
    if (shadow.castShadow && surface.shadowFade > 0.01f)
    {
        float3 normalBias = surface.interpolatedNormal * shadow.shadowBias *
            dot(surface.position - light.Position, normalize(light.Direction));
        
        float4 posLS = mul(float4(surface.position + normalBias, 1.0f), shadow.shadowTransform);
        posLS /= posLS.w;
        
        // PCF
        float weights[FILTER_SAMPLER_SIZE];
        float2 positions[FILTER_SAMPLER_SIZE];
        float4 size = GetSpotShadowMapSize(index);
        FILTER_SETUP(size, posLS.xy, weights, positions);
        
        attenuation = 0.0f;  
        [unroll]
        for (uint i = 0; i < FILTER_SAMPLER_SIZE; i++)
        {
            attenuation += weights[i] * GetSpotShadowMapValue(index, positions[i], posLS.z);
        }
    }
    
    return lerp(1.0f, attenuation, surface.shadowFade);
}

// Point Shadow
float GetPointShadowAttenuation(Surface surface, uint index)
{
    float attenuation = 1.0f;
    PointLight light = _PointLights[index];
    PointShadow shadow = _ShadowPoint[index];
    
    if(shadow.castShadow && surface.shadowFade > 0.01f)
    {
        float3 lightToSurface = surface.position - light.Position;
        float3 rayDir = normalize(lightToSurface);
        
        float3 planeNormal = GetCubeFaceNormal(lightToSurface);
        float bias = shadow.shadowBias * dot(lightToSurface, planeNormal);
        
        float rayCompareLength = light.InvRange * (distance(surface.position, light.Position) - bias);
                
        // PCF
        float weights[FILTER_SAMPLER_SIZE];
        float2 offset[FILTER_SAMPLER_SIZE];
        float4 size = GetPointShadowMapSize(index);
        FILTER_SETUP(size, 0.0f, weights, offset);
        
        attenuation = 0.0f;
        [unroll]
        for (uint i = 0; i < FILTER_SAMPLER_SIZE; i++)
        {
            float3 dir = GetCubeShadowSamplingDirection(rayDir, offset[i]);
            attenuation += weights[i] * GetPointShadowMapValue(index, dir, rayCompareLength);
        }
        
    }
    
    return lerp(1.0f, attenuation, surface.shadowFade);
}

// Directional Shadow
float3 FilterSunShadow(uint cascadeIndex, float2 uv, float compareDepth)
{
    float weights[FILTER_SAMPLER_SIZE];
    float2 positions[FILTER_SAMPLER_SIZE];
    float4 size = GetSunCascadeShadowMapSize(cascadeIndex);
    FILTER_SETUP(size, uv, weights, positions);
    
    float attenuation = 0.0f;
    
    [unroll]
    for (uint i = 0; i < FILTER_SAMPLER_SIZE; i++)
    {
        attenuation += weights[i] * GetSunShadowMapValue(cascadeIndex, positions[i], compareDepth);
    }
       
    // visualize cascade index for test    
    float3 color;
    if (cascadeIndex == 0)
    {
        color = attenuation * float3(1.0f, 0.0f, 0.0f);
    }
    else if(cascadeIndex == 1)
    {
        color = attenuation * float3(0.0f, 1.0f, 0.0f);
    }
    else if (cascadeIndex == 2)
    {
        color = attenuation * float3(0.0f, 0.0f, 1.0f);
    }
    else
    {
        color = attenuation * float3(1.0f, 1.0f, 0.0f);
    }
    
    return color;
    
    //return attenuation;
}

void GetSunShadowCascade(float depth, out uint cascadeIndex, out float cascadeStrength)
{
    cascadeIndex = 0;
    cascadeStrength = 1.0f;
    float3 attenuation = 1.0f;
    
    [loop]
    for (; cascadeIndex < _CascadeCount; cascadeIndex++)
    {
        float bound = _ShadowCascade[cascadeIndex].cascadeDistance;
        if (depth < bound)
        {
            cascadeStrength = ShadowFade(depth, bound, 10.0f);
            break;
        }
    }
}

float3 GetSunShadowAttenuation(Surface surface)
{
    float3 attenuation = 1.0f;
    float shadowStrength = surface.shadowFade;
    
    if (_SunCastShadow == 1 && surface.shadowFade > 0.01f)
    {
        uint cascadeIndex;
        float cascadeStrength;
        GetSunShadowCascade(surface.depth, cascadeIndex, cascadeStrength);
        CascadeShadow shadow = _ShadowCascade[cascadeIndex];
        
        float3 normalBias = shadow.shadowBias * surface.interpolatedNormal;
        float4 posLS = mul(float4(surface.position + normalBias, 1.0f), shadow.shadowTransform);
        posLS /= posLS.w;
        
        attenuation = FilterSunShadow(cascadeIndex, posLS.xy, posLS.z);
        
        if (cascadeIndex + 1 < _CascadeCount && cascadeStrength < 1.0f)
        {
            shadow = _ShadowCascade[cascadeIndex + 1];
            normalBias = shadow.shadowBias * surface.interpolatedNormal;
            posLS = mul(float4(surface.position + normalBias, 1.0f), shadow.shadowTransform);
            posLS /= posLS.w;
            
            attenuation = lerp(
                FilterSunShadow(cascadeIndex + 1, posLS.xy, posLS.z),
                attenuation,
                cascadeStrength);
        }
        else
        {
            shadowStrength *= cascadeStrength;
        }

    }
    
    return lerp(1.0f, attenuation, shadowStrength);
    
}
#endif