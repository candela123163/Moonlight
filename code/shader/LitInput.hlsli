#ifndef _LIT_INPUT_INCLUDE
#define _LIT_INPUT_INCLUDE

// --------- static samplers ------------------------------------
#include "StaticSamplers.hlsli"
// ----------------------------------------------------------------


// --------- textures ---------------------------------------------
#define TEXTURE_ARRAY_SIZE 128
Texture2D _2DMaps[TEXTURE_ARRAY_SIZE] : register(t0, space0);
TextureCube _CubeMaps[TEXTURE_ARRAY_SIZE] : register(t0, space1);
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
}

DEFINE_SHADOW_CONSTANT(b5)

// ---------------------------------------------------------------


// ------------------ getters ------------------------------------
float4 GetBaseColor(float2 uv)
{
    return _2DMaps[_AlbedoMapIndex].Sample(_SamplerAnisotropicWrap, uv) * _AlbedoFactor;
}

float3 GetNormalTS(float2 uv)
{
    float3 rawNormal = _2DMaps[_NormalMapIndex].Sample(_SamplerAnisotropicWrap, uv).rgb;
    rawNormal = rawNormal * 2.0f - 1.0f;
    rawNormal.xy *= _NormalScale;
    return normalize(rawNormal);
}

float GetMetallic(float2 uv)
{
    return _2DMaps[_MetalRoughMapIndex].Sample(_SamplerAnisotropicWrap, uv).b * _MetallicFactor;
}

float GetRoughness(float2 uv)
{
    return _2DMaps[_MetalRoughMapIndex].Sample(_SamplerAnisotropicWrap, uv).g * _RoughnessFactor;
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
    return _CubeMaps[_PrefilterMapIndex].SampleLevel(_SamplerLinearClamp, dir, roughness * _PrefilterMapMipCount);
}

float GetSpotShadowMapValue(uint index, float2 uv, float compareDepth)
{
    uint mapIndex = _ShadowSpot[index].shadowMapIndex;
#ifdef REVERSE_Z
    return _2DMaps[mapIndex].SampleCmpLevelZero(_SamplerShadowGreater, uv, compareDepth).r;
#else
    return _2DMaps[mapIndex].SampleCmpLevelZero(_SamplerShadowLess, uv, compareDepth).r;
#endif
}

float GetSunShadowMapValue(uint cascade, float2 uv, float compareDepth)
{
    uint mapIndex = _ShadowCascade[cascade].shadowMapIndex;
#ifdef REVERSE_Z
    return _2DMaps[mapIndex].SampleCmpLevelZero(_SamplerShadowGreater, uv, compareDepth).r;
#else
    return _2DMaps[mapIndex].SampleCmpLevelZero(_SamplerShadowLess, uv, compareDepth).r;
#endif
}

float GetPointShadowMapValue(uint index, float3 dir, float compareDepth)
{
    uint mapIndex = _ShadowPoint[index].shadowMapIndex;
#ifdef REVERSE_Z
    return _CubeMaps[mapIndex].SampleCmpLevelZero(_SamplerShadowGreater, dir, compareDepth).r;
#else
    return _CubeMaps[mapIndex].SampleCmpLevelZero(_SamplerShadowLess, dir, compareDepth).r;
#endif
}

float4 GetSpotShadowMapSize(uint index)
{
    uint mapIndex = _ShadowSpot[index].shadowMapIndex;
    float w, h;
    _2DMaps[mapIndex].GetDimensions(w, h);
    return float4(1.0f / w, 1.0f / h, w, h);
}

float4 GetPointShadowMapSize(uint index)
{
    uint mapIndex = _ShadowPoint[index].shadowMapIndex;
    float w, h;
    _CubeMaps[mapIndex].GetDimensions(w, h);
    return float4(1.0f / w, 1.0f / h, w, h);
}

float4 GetSunCascadeShadowMapSize(uint cascade)
{
    uint mapIndex = _ShadowCascade[cascade].shadowMapIndex;
    float w, h;
    _2DMaps[mapIndex].GetDimensions(w, h);
    return float4(1.0f / w, 1.0f / h, w, h);
}
// ---------------------------------------------------------------
#endif