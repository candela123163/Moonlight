#ifndef _GI_INCLUDE
#define _GI_INCLUDE
#include "LitInput.hlsli"
#include "LitDefine.hlsli"

EnvLight GetEnvLight(Surface surface)
{
    float SSAO = GetSSAO(surface.screenUV);
    
    float3 L = reflect(-surface.viewDir, surface.normal);
    EnvLight envLight;
    envLight.irradiance = GetEnvIrradiance(surface.normal) * SSAO * surface.ao;
    envLight.prefilteredColor = GetEnvPrefilteredColor(L, surface.roughness);
    
    return envLight;
}

#endif