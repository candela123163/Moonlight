#ifndef _SURFACE_INCLUDED
#define _SURFACE_INCLUDED

struct Surface
{
    float3 position;
    float3 normal;
    float3 interpolatedNormal;
    float3 viewDir;
    float depth;
    float3 albedo;
    float alpha;
    float metallic;
    float roughness;
    float shadowFade;
    float2 screenUV;
};

struct Light
{
    float3 color;
    float intensity;
    float3 direction;
    float attenuation;
};

struct EnvLight
{
    float3 irradiance;
    float3 prefilteredColor;
};

struct BRDF
{
    float3 specular;
    float3 diffuse;
};
#endif