#ifndef _LIGHT_INCLUDED
#define _LIGHT_INCLUDED

#include "LitInput.hlsli"
#include "Surface.hlsli"
#include "Util.hlsli"

struct Light
{
    float3 color;
    float intensity;
    float3 direction;
    float attenuation;
};


Light GetSun()
{
    Light light;
    light.color = _SunColor;
    light.intensity = _SunIntensity;
    light.direction = -normalize(_SunDirection);
    light.attenuation = 1.0f;
    
    return light;
}

int GetPointLightCount()
{
    return _PointLightCount;
}

Light GetPointLight(int index, Surface surface)
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
            1.0f - Square(squareDistance * pointLight.InvSquareRange)
        )
    ) / squareDistance;
    light.attenuation = rangeAttenuation;
    
    return light;
}

int GetSpotLightCount()
{
    return _SpotLightCount;
}

Light GetSpotLight(int index, Surface surface)
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
            1.0f - Square(squareDistance * spotLight.InvSquareRange)
        )
    ) / squareDistance;
    float spotAttenuation =
    Square(
        saturate(dot(spotLight.Direction, light.direction)) * spotLight.AttenuationFactorA
        + spotLight.AttenuationFactorB
    );
    light.attenuation = rangeAttenuation * spotAttenuation;
    
    return light;
}



#endif