#ifndef _GI_INCLUDE
#define _GI_INCLUDE
#include "LitInput.hlsli"

struct EnvLight
{
    float3 irradiance;
    float3 prefilteredColor;
};

EnvLight GetEnvLight(float3 v, float3 n, float roughness)
{
    float3 L = reflect(-v, n);
    EnvLight envLight;
    envLight.irradiance = GetEnvIrradiance(n);
    envLight.prefilteredColor = GetEnvPrefilteredColor(L, roughness);
    
    return envLight;
}

#endif