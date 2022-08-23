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
};

#endif