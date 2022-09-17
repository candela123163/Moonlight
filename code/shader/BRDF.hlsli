#ifndef _BRDF_INCLUDE
#define _BRDF_INCLUDE

#include "Util.hlsli"
#include "LitDefine.hlsli"
#include "LitInput.hlsli"

// Shading Model: Cook¨C TorranceBRDF
// D: GGX
// F: Schlick Approximate
// G: Schlick-GGX
//
// f_r = kd * f_lambert + ks * f_cook_torrance
// f_lambert = albedo / Pi
// f_cook_torrance = D * F * G / (4 * dot(wi, n) * dot(wo, n))
// k_s = F
// k_d = (1 - F) * (1 - metallic)



// --------------------------- F ------------------------------
#define MIN_REFLECTION 0.04f

float3 MetalWorkflowF0(float3 albedo, float metallic)
{
    return lerp(MIN_REFLECTION, albedo, metallic);
}

float3 FresnelEpicApproximate(float3 w, float3 h, float3 f0)
{
    float wDotH = saturate(dot(w, h));
    return f0 + (1.0f - f0) * exp2(wDotH * (-5.55473f * wDotH - 6.98315f));
}

float3 FresnelSchlickApproximate(float3 w, float3 h, float3 f0)
{
    float wDotH = saturate(dot(w, h));
    return f0 + (1.0f - f0) * pow(1.0f - wDotH, 5.0f);
}

float3 Fresnel(float3 w, float3 h, float3 f0)
{
    return FresnelEpicApproximate(w, h, f0);
}

float3 FresnelSchlickRoughness(float3 w, float3 h, float3 f0, float roughness)
{
    float wDotH = saturate(dot(w, h));
    float3 r1 = max(f0, 1.0f - roughness);
    return f0 + (r1 - f0) * pow(1.0f - wDotH, 5.0f);
}
// ----------------------------------------------------------

// ---------------------------- D ---------------------------
float GGX_D(float3 n, float3 h, float roughness)
{
    float nDotH = saturate(dot(n, h));
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float cos2Theta = nDotH * nDotH;
    float t = max(0.001f, (alpha2 - 1.0f) * cos2Theta + 1.0f);
    return alpha2 / (PI * t * t);
}

float3 GGX_Sample(float2 Xi, float roughness)
{
    float alpha = roughness * roughness;
    float phi = TWO_PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (alpha * alpha - 1.0f) * Xi.y));
    float sinTheta = 1.0f - cosTheta * cosTheta;
    
    float3 h = float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
    return h;
}

// ----------------------------------------------------------

// ---------------------------- G ---------------------------
float SchlickGGX_G1(float3 w, float3 n, float roughness)
{
    float k = pow(roughness + 1.0f, 2.0f) / 8.0f;
    float wdotN = saturate(dot(w, n));
    return wdotN / (wdotN * (1.0f - k) + k);
}

float SchlickGGX_G2(float3 wi, float3 wo, float3 n, float roughness)
{
    return SchlickGGX_G1(wi, n, roughness) * SchlickGGX_G1(wo, n, roughness);
}
// ----------------------------------------------------------


// --------------------- Direct BRDF ------------------------
float3 CookTorranceSpecular(Surface surface, float3 lightDir, out float3 Ks)
{
    float3 h = normalize(surface.viewDir + lightDir);
    float D = GGX_D(surface.normal, h, surface.roughness);
    float3 F = Fresnel(surface.viewDir, h, MetalWorkflowF0(surface.albedo, surface.metallic));
    float G = SchlickGGX_G2(lightDir, surface.viewDir, surface.normal, surface.roughness);
    Ks = F;
    float ndotV = saturate(dot(surface.normal, surface.viewDir));
    float ndotL = saturate(dot(surface.normal, lightDir));
    return (D * F * G) / (4.0f * max(0.001f, ndotV * ndotL));
}

float3 LambertDiffuse(Surface surface, float3 Ks)
{
    float3 Kd = (1.0f - Ks) * (1.0f - surface.metallic);
    return Kd * surface.albedo * INV_PI;    
}

BRDF DirectBRDF(Surface surface, float3 lightDir)
{
    BRDF brdf;
    float3 Ks;
    brdf.specular = CookTorranceSpecular(surface, lightDir, Ks);
    brdf.diffuse = LambertDiffuse(surface, Ks);
    return brdf;
}
// ----------------------------------------------------------

// --------------------- Indirect BRDF ----------------------
BRDF IndirectBRDF(Surface surface)
{
    BRDF brdf;
    float3 f0 = MetalWorkflowF0(surface.albedo, surface.metallic);
    float3 F = FresnelSchlickRoughness(surface.viewDir, surface.normal, f0, surface.roughness);
    float3 Ks = F;
    float3 Kd = (1.0f - Ks) * (1.0f - surface.metallic);
    float2 LUTuv = float2(saturate(dot(surface.normal, surface.viewDir)), 1.0f - surface.roughness);
    float2 specularBRDFFactor = GetEnvSpecularBRDFFactor(LUTuv);
    
    brdf.specular = f0 * specularBRDFFactor.x + specularBRDFFactor.yyy;
    brdf.diffuse = surface.albedo * Kd;
    
    return brdf;
}
// ----------------------------------------------------------

#endif