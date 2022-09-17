#ifndef _LIT_INPUT_INCLUDE
#define _LIT_INPUT_INCLUDE

// --------- static samplers ------------------------------------
#include "StaticSamplers.hlsli"
// ----------------------------------------------------------------


// --------- textures ---------------------------------------------
Texture2D _2DMaps[] : register(t0, space0);
TextureCube _CubeMaps[] : register(t0, space1);
// ----------------------------------------------------------------


// --------- constant buffers ------------------------------------
#include "Constant.hlsli"
DEFINE_OBJ_CONSTANT(b0)
DEFINE_MATERIAL_CONSTANT(b1)
DEFINE_CAMERA_CONSTANT(b2)
DEFINE_LIGHT_CONSTANT(b3)

cbuffer IBLConstant : register(b4)
{
    int _IrradianceMapIndex;
    int _PrefilterMapIndex;
    int _BRDFLUTIndex;
    int _PrefilterMapMipCount;
    
    int _SSAOMapIndex;
    int _IBLEnable;
}

DEFINE_SHADOW_CONSTANT(b5)
DEFINE_RENDER_TARGET_PARAM_CONSTANT(b6)
// ---------------------------------------------------------------

#define MIN_ROUGHNESS 0.05
#define MAX_ROUGHNESS 0.95
// ------------------ getters ------------------------------------
float4 GetBaseColor(float2 uv)
{
    return _2DMaps[_AlbedoMapIndex].Sample(_SamplerLinearWrap, uv) * _AlbedoFactor;
}

float3 GetNormalTS(float2 uv)
{
    float3 rawNormal = _2DMaps[_NormalMapIndex].Sample(_SamplerLinearWrap, uv).rgb;
    rawNormal = rawNormal * 2.0f - 1.0f;
    rawNormal.xy *= _NormalScale;
    return normalize(rawNormal);
}

float GetMetallic(float2 uv)
{
    if (_UseMetalRoughness)
    {
        return _2DMaps[_MetalRoughMapIndex].Sample(_SamplerLinearWrap, uv).b * _MetallicFactor;
    }
    else
    {
        return 0.0f;
    }
}

float GetRoughness(float2 uv)
{
    if(_UseMetalRoughness)
    {
        return clamp(_2DMaps[_MetalRoughMapIndex].Sample(_SamplerLinearWrap, uv).g * _RoughnessFactor, MIN_ROUGHNESS, MAX_ROUGHNESS);
    }
    else
    {
        return 1.0f;
    }
}

bool NormalMapped()
{
    return _NormalMapped == 1;
}

float2 GetEnvSpecularBRDFFactor(float2 uv)
{
    return _2DMaps[_BRDFLUTIndex].Sample(_SamplerLinearClamp, uv).xy;
}

float3 GetEnvIrradiance(float3 dir)
{
    return _CubeMaps[_IrradianceMapIndex].Sample(_SamplerLinearClamp, dir).rgb;
}

float3 GetEnvPrefilteredColor(float3 dir, float roughness)
{
    roughness *= 1.7f - 0.7 * roughness;
    return _CubeMaps[_PrefilterMapIndex].SampleLevel(_SamplerLinearClamp, dir, roughness * (_PrefilterMapMipCount - 1)).rgb;
}

float GetSpotShadowMapValue(uint index, float2 uv, float compareDepth)
{
    uint mapIndex = _ShadowSpot[index].shadowMapIndex;
#ifdef REVERSE_Z
    return _2DMaps[NonUniformResourceIndex(mapIndex)].SampleCmpLevelZero(_SamplerShadowGreater, uv, compareDepth).r;
#else
    return _2DMaps[NonUniformResourceIndex(mapIndex)].SampleCmpLevelZero(_SamplerShadowLess, uv, compareDepth).r;
#endif
}

float GetSunShadowMapValue(uint cascade, float2 uv, float compareDepth)
{
    uint mapIndex = _ShadowCascade[cascade].shadowMapIndex;
#ifdef REVERSE_Z
    return _2DMaps[NonUniformResourceIndex(mapIndex)].SampleCmpLevelZero(_SamplerShadowGreater, uv, compareDepth).r;
#else
    return _2DMaps[NonUniformResourceIndex(mapIndex)].SampleCmpLevelZero(_SamplerShadowLess, uv, compareDepth).r;
#endif
}

float GetPointShadowMapValue(uint index, float3 dir, float compareDepth)
{
    uint mapIndex = _ShadowPoint[index].shadowMapIndex;
    return _CubeMaps[NonUniformResourceIndex(mapIndex)].SampleCmpLevelZero(_SamplerShadowLess, dir, compareDepth).r;

}

float4 GetSpotShadowMapSize(uint index)
{
    uint mapIndex = _ShadowSpot[index].shadowMapIndex;
    float w, h;
    _2DMaps[NonUniformResourceIndex(mapIndex)].GetDimensions(w, h);
    return float4(1.0f / w, 1.0f / h, w, h);
}

float4 GetPointShadowMapSize(uint index)
{
    uint mapIndex = _ShadowPoint[index].shadowMapIndex;
    float w, h;
    _CubeMaps[NonUniformResourceIndex(mapIndex)].GetDimensions(w, h);
    return float4(1.0f / w, 1.0f / h, w, h);
}

float4 GetSunCascadeShadowMapSize(uint cascade)
{
    uint mapIndex = _ShadowCascade[cascade].shadowMapIndex;
    float w, h;
    _2DMaps[NonUniformResourceIndex(mapIndex)].GetDimensions(w, h);
    return float4(1.0f / w, 1.0f / h, w, h);
}

float GetSSAO(float2 uv)
{
    return saturate(_2DMaps[_SSAOMapIndex].SampleLevel(_SamplerLinearClamp, uv, 0).r);
}

float GetTexturedAO(float2 uv)
{
    if (_UseAO)
    {
        return saturate(_2DMaps[_AOMapIndex].Sample(_SamplerLinearWrap, uv).r);
    }
    else
    {
        return 1.0f;
    }
}

float3 GetEmissive(float2 uv)
{
    if(_UseEmissive)
    {
        return _2DMaps[_EmissiveMapIndex].Sample(_SamplerLinearWrap, uv).rgb * _EmissiveFactor;
    }
    else
    {
        return (0.0f).rrr;
    }
}
// ---------------------------------------------------------------
#endif