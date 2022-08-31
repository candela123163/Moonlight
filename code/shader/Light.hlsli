#ifndef _LIGHT_INCLUDED
#define _LIGHT_INCLUDED

#include "LitInput.hlsli"
#include "LitDefine.hlsli"
#include "Util.hlsli"
#include "Shadow.hlsli"

Light GetSun(Surface surface)
{
    Light light;
    light.color = _SunColor;
    light.intensity = _SunIntensity;
    light.direction = -normalize(_SunDirection);
    light.attenuation = GetSunShadowAttenuation(surface);
    
    return light;
}

int GetPointLightCount()
{
    return _PointLightCount;
}

Light GetPointLight(uint index, Surface surface)
{
    Light light;
    PointLight pointLight = _PointLights[index];
    
    light.color = pointLight.Color;
    light.intensity = pointLight.Intensity;
    float3 ray = pointLight.Position - surface.position;
    light.direction = normalize(ray);
    float squareDistance = max(0.0001f, dot(ray, ray));
    float rangeAttenuation = 
    Square(
        saturate(
            1.0f - Square(squareDistance * pointLight.InvRange * pointLight.InvRange)
        )
    ) / squareDistance;
    
    light.attenuation = rangeAttenuation;
    
    [branch]
    if (light.attenuation.r > 0.01f)
    {
        light.attenuation *= GetPointShadowAttenuation(surface, index);
    }
    
    return light;
}

int GetSpotLightCount()
{
    return _SpotLightCount;
}

Light GetSpotLight(uint index, Surface surface)
{
    Light light;
    SpotLight spotLight = _SpotLights[index];
    
    light.color = spotLight.Color;
    light.intensity = spotLight.Intensity;
    float3 ray = spotLight.Position - surface.position;
    light.direction = normalize(ray);
    float squareDistance = max(0.0001f, dot(ray, ray));
    float rangeAttenuation = 
        Square(
            saturate(
                1.0f - Square(squareDistance * spotLight.InvRange * spotLight.InvRange)
            )
        ) / squareDistance;
    float spotAttenuation = 
        Square(
            saturate(dot(-normalize(spotLight.Direction), light.direction) * spotLight.AttenuationFactorA
            + spotLight.AttenuationFactorB)
        );
    
    light.attenuation = rangeAttenuation * spotAttenuation;
    
    [branch]
    if (light.attenuation.r > 0.01f)
    {
        light.attenuation *= GetSpotShadowAttenuation(surface, index);
    }
    return light;
}



#endif