#ifndef _LIT_INPUT_INCLUDE
#define _LIT_INPUT_INCLUDE

// --------- static samplers ------------------------------------
#include "StaticSamplers.hlsli"
// ----------------------------------------------------------------


// --------- textures ---------------------------------------------
#define TEXTURE_ARRAY_SIZE 256
Texture2D _2DMaps[TEXTURE_ARRAY_SIZE] : register(t0, space0);
TextureCube _CubeMaps[TEXTURE_ARRAY_SIZE] : register(t0, space1);
// ----------------------------------------------------------------


// --------- constant buffers ------------------------------------
#include "Constant.hlsli"
DEFINE_OBJ_CONSTANT(b0)
DEFINE_MATERIAL_CONSTANT(b1)
DEFINE_CAMERA_CONSTANT(b2)
DEFINE_LIGHT_CONSTANT(b3)
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
    return float2(0.0f, 0.0f);
}

// ---------------------------------------------------------------
#endif