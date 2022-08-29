#ifndef _SHADOW_INCLUDE
#define _SHADOW_INCLUDE

#include "LitInput.hlsli"
#include "LitDefine.hlsli"
#include "ShadowSamplingTent.hlsli"
#include "Util.hlsli"

#define FILTER_SAMPLER_SIZE 16
#define FILTER_SETUP SampleShadow_ComputeSamples_Tent_7x7

float GetShadowGlobalFade(float depth)
{
    return 1.0f;
}


// Spot Shadow
float GetSpotShadowAttenuation(Surface surface, uint index)
{
    float attenuation = 1.0f;
    if (surface.shadowFade > 0.01f)
    {
        SpotLight light = _SpotLights[index];
        SpotShadow shadow = _ShadowSpot[index];
        
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
    if(surface.shadowFade > 0.01f)
    {
        PointLight light = _PointLights[index];
        PointShadow shadow = _ShadowPoint[index];
        
        float3 lightToSurface = surface.position - light.Position;
        float3 rayDir = normalize(lightToSurface);
        
        float3 planeNormal = GetCubeFaceNormal(lightToSurface);
        float bias = shadow.shadowBias * dot(lightToSurface, planeNormal);
        
        float rayCompareLength = light.InvRange * (distance(surface.position, light.Position) - bias);
        
        //attenuation = GetPointShadowMapValue(index, rayDir, rayCompareLength);
        
        // PCF
        float weights[FILTER_SAMPLER_SIZE];
        float2 offset[FILTER_SAMPLER_SIZE];
        float4 size = GetPointShadowMapSize(index);
        FILTER_SETUP(size, 0.0f, weights, offset);
        
        attenuation = 0.0f;
        for (uint i = 0; i < FILTER_SAMPLER_SIZE; i++)
        {
            float3 dir = GetCubeShadowSamplingDirection(rayDir, offset[i]);
            attenuation += weights[i] * GetPointShadowMapValue(index, dir, rayCompareLength);
        }
        
    }
    
    return lerp(1.0f, attenuation, surface.shadowFade);
}
#endif