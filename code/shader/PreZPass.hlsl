#include  "StaticSamplers.hlsli"
#include "Constant.hlsli"
#include "Util.hlsli"

Texture2D _2DMaps[] : register(t0);

DEFINE_OBJ_CONSTANT(b0)
DEFINE_MATERIAL_CONSTANT(b1)
DEFINE_CAMERA_CONSTANT(b2)


struct VertexIn
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float4 unjitteredPosH : VAR_POSITION_W;
    float4 preUnjitteredPosH : VAR_PRE_POSITION_W;
    float3 normalV : VAR_NORMAL_V;
    float2 uv : VAR_TEXCOORD;
};


VertexOut vs(VertexIn vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    float4 prePosW = mul(float4(vin.posL, 1.0f), _PreWorld);
    
    vout.posH = mul(posW, _ViewProj);
    vout.unjitteredPosH = mul(posW, _UnjitteredViewProj);
    vout.preUnjitteredPosH = mul(prePosW, _UnjitteredPreViewProj);
    
    float3 normalW = mul(vin.normalL, (float3x3) _InvTransposeWorld);

    // reverse to apply transpose to _InvView
    vout.normalV = mul((float3x3) _InvView, normalW);
        
    vout.uv = vin.uv;
        
    return vout;
}

struct PSOut
{
    float4 normal : SV_Target0;
    float2 motion : SV_Target1;
};


PSOut ps(VertexOut pin)
{
    float alpha = (_2DMaps[_AlbedoMapIndex].Sample(_SamplerAnisotropicWrap, pin.uv) * _AlbedoFactor).a;
    clip(alpha - 0.5f);
    
    PSOut psout;
    
    psout.normal = float4(EncodeNormal(normalize(pin.normalV)), 1.0f);

    pin.unjitteredPosH /= pin.unjitteredPosH.w;
    float2 unjitteredUV = NDCXYToUV(pin.unjitteredPosH.xy);
    pin.preUnjitteredPosH /= pin.preUnjitteredPosH.w;
    float2 preUnjitteredUV = NDCXYToUV(pin.preUnjitteredPosH.xy);
    
    psout.motion = unjitteredUV - preUnjitteredUV;
        
    return psout;
}