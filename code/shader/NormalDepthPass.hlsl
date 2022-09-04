#include  "StaticSamplers.hlsli"
#include "Constant.hlsli"

DEFINE_OBJ_CONSTANT(b0)
DEFINE_MATERIAL_CONSTANT(b1)
DEFINE_CAMERA_CONSTANT(b2)

#define TEXTURE_ARRAY_SIZE 128
Texture2D _2DMaps[TEXTURE_ARRAY_SIZE] : register(t0);

struct VertexIn
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float3 tangentL : TANGENT;
    float3 biTangentL : BITANGENT;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posW : VAR_POSITION_W;
    float3 normalV : VAR_NORMAL_V;
    float3 tangentV : VAR_TANGENT_V;
    float3 biTangentV : VAR_BITANGENT_V;
    float2 uv : VAR_TEXCOORD;
};


VertexOut vs(VertexIn vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    vout.posH = mul(posW, _ViewProj);
    vout.posW = posW.xyz;
    
    float3 normalW = mul(vin.normalL, (float3x3) _InvTransposeWorld);
    float3 tangentW = mul(vin.tangentL, (float3x3) _World);
    float3 biTangentW = mul(vin.biTangentL, (float3x3) _World);
    
    // reverse to apply transpose to _InvView
    vout.normalV = mul((float3x3) _InvView, normalW);
    vout.tangentV = mul(tangentW, (float3x3) _View);
    vout.biTangentV = mul(biTangentW, (float3x3) _View);
    
    vout.uv = vin.uv;
    
    return vout;
}

float4 ps(VertexOut pin) : SV_TARGET
{
    pin.normalV = normalize(pin.normalV);
    pin.tangentV = normalize(pin.tangentV);
    pin.biTangentV = normalize(pin.biTangentV);
    
    float alpha = (_2DMaps[_AlbedoMapIndex].Sample(_SamplerAnisotropicWrap, pin.uv) * _AlbedoFactor).a;
    
    clip(alpha - 0.5f);
    
    float3 normalV;
    if (_NormalMapped == 1)
    {
        float3 rawNormal = _2DMaps[_NormalMapIndex].Sample(_SamplerAnisotropicWrap, pin.uv).rgb;
        rawNormal = rawNormal * 2.0f - 1.0f;
        rawNormal.xy *= _NormalScale;
        float3 normalTS = normalize(rawNormal);
        normalV = mul(normalTS, float3x3(pin.tangentV, pin.biTangentV, pin.normalV));
    }
    else
    {
        normalV = pin.normalV;
    }
    
    return float4(normalV, 1.0f);
}