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
    float3 energy = IncomingLight(surface, light);
    
    [branch]
    if (dot(energy, 1.0f) > EPSILON)
    {
        BRDF brdf = DirectBRDF(surface, light.direction);
        energy *= (brdf.diffuse + brdf.specular);
    }
    
    return energy;
}

float3 EnvShading(Surface surface)
{
    EnvLight envLight = GetEnvLight(surface);
    BRDF indirectBRDF = IndirectBRDF(surface);
    return indirectBRDF.diffuse * envLight.irradiance + indirectBRDF.specular * envLight.prefilteredColor;
}

#endif