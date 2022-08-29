#ifndef _SHADOW_INCLUDE
#define _SHADOW_INCLUDE

#include "LitInput.hlsli"
#include "LitDefine.hlsli"
#include "ShadowSamplingTent.hlsli"

#define FILTER_SAMPLER_SIZE 9
#define FILTER_SETUP SampleShadow_ComputeSamples_Tent_5x5

float GetShadowGlobalFade(float depth)
{
    return 1.0f;
}

float GetDistanceToLightPlane(float3 pos, float3 lightPos, float3 lightPlaneNormal)
{
    return dot(pos - lightPos, lightPlaneNormal);
}

// Spot Shadow
float GetSpotShadowAttenuation(Surface surface, uint index)
{
    float attenuation = 1.0f;
    if (surface.shadowFade > 0.01f)
    {
        float3 normalBias = surface.interpolatedNormal *
        _ShadowSpot[index].normalBias *
        GetDistanceToLightPlane(surface.position, _SpotLights[index].Position, normalize(_SpotLights[index].Direction));
        float4 posLS = mul(float4(surface.position + normalBias, 1.0f), _ShadowSpot[index].shadowTransform);
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

#endif