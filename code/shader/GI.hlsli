#ifndef _GI_INCLUDE
#define _GI_INCLUDE
#include "LitInput.hlsli"
#include "LitDefine.hlsli"

EnvLight GetEnvLight(Surface surface)
{
    float ao = GetAO(surface.screenUV);
    float3 L = reflect(-surface.viewDir, surface.normal);
    EnvLight envLight;
    envLight.irradiance = GetEnvIrradiance(surface.normal) * ao;
    envLight.prefilteredColor = GetEnvPrefilteredColor(L, surface.roughness);
    
    return envLight;
}

#endif