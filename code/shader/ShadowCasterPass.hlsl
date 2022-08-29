#include "StaticSamplers.hlsli"
#include "Constant.hlsli"

DEFINE_OBJ_CONSTANT(b0)
DEFINE_MATERIAL_CONSTANT(b1)
DEFINE_SHADOW_CASTER_CONSTANT(b2)

#define TEXTURE_ARRAY_SIZE 128
Texture2D _2DMaps[TEXTURE_ARRAY_SIZE] : register(t0);

struct VertexIn
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct v2f_shadow
{
    float4 posH : SV_POSITION;
    float2 uv : VAR_TEXCOORD;
};

v2f_shadow vs_shadow(VertexIn vin)
{
    v2f_shadow vout;
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    vout.posH = mul(posW, _LightViewProject);
    vout.uv = vin.uv;
    
    return vout;
}

void ps_shadow(v2f_shadow pin)
{
    float alpha = (_2DMaps[_AlbedoMapIndex].Sample(_SamplerAnisotropicWrap, pin.uv) * _AlbedoFactor).a;
    clip(alpha - 0.5f);
}

struct v2f_point_shadow
{
    float4 posH : SV_POSITION;
    float3 posW : VAR_POSITION_W;
    float2 uv : VAR_TEXCOORD;
};

v2f_point_shadow vs_point_shadow(VertexIn vin)
{
    v2f_point_shadow vout;
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    vout.posH = mul(posW, _LightViewProject);
    vout.posW = posW.xyz;
    vout.uv = vin.uv;
    
    return vout;
}

float ps_point_shadow(v2f_point_shadow pin) : SV_TARGET
{
    float alpha = (_2DMaps[_AlbedoMapIndex].Sample(_SamplerAnisotropicWrap, pin.uv) * _AlbedoFactor).a;
    clip(alpha - 0.5f);
        
    return distance(pin.posW, _LightPosition.xyz) * _LightInvRange;
}