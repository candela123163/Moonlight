#ifndef _LIGHTING_INCLUDE
#define _LIGHTING_INCLUDE

#include "Light.hlsli"
#include "BRDF.hlsli"
#include "GI.hlsli"

float3 IncomingLight(Surface surface, Light light)
{
    return saturate(dot(surface.normal, light.direction) * light.attenuation) * light.color * light.intensity;
}

float3 Shading(Surface surface, Light light)
{
    BRDF brdf = DirectBRDF(surface, light.direction);
    return IncomingLight(surface, light) * (brdf.diffuse + brdf.specular);
}

float3 EnvShading(Surface surface)
{
    EnvLight envLight = GetEnvLight(surface.viewDir, surface.normal, surface.roughness);
    BRDF indirectBRDF = IndirectBRDF(surface);
    return indirectBRDF.diffuse * envLight.irradiance + indirectBRDF.specular * envLight.prefilteredColor;
}

#endif