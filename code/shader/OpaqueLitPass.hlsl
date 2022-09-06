#include "Lighting.hlsli"


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
    float3 normalW : VAR_NORMAL_W;
    float3 tangentW : VAR_TANGENT_W;
    float3 biTangentW : VAR_BITANGENT_W;
    float2 uv : VAR_TEXCOORD;
    
    float4 test : VVV;
};


VertexOut vs(VertexIn vin)
{
    VertexOut vout;
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    vout.posH = mul(posW, _ViewProj);
    vout.posW = posW.xyz;
    vout.normalW = mul(vin.normalL, (float3x3) _InvTransposeWorld);
    vout.tangentW = mul(vin.tangentL, (float3x3) _World);
    vout.biTangentW = mul(vin.biTangentL, (float3x3) _World);
    vout.uv = vin.uv;
    
    vout.test = vout.posH;
    
    return vout;
}


float4 ps(VertexOut pin) : SV_TARGET
{   
    pin.normalW = normalize(pin.normalW);
    pin.tangentW = normalize(pin.tangentW);
    pin.biTangentW = normalize(pin.biTangentW);
    
    float4 baseColor = GetBaseColor(pin.uv);
    
    clip(baseColor.a - 0.5f);
    
    float3 normalW;
    if (NormalMapped())
    {
        normalW = NormalTangentToWorld(GetNormalTS(pin.uv), pin.tangentW, pin.biTangentW, pin.normalW);
    }
    else
    {
        normalW = pin.normalW;
    }

    Surface surface;
    surface.position = pin.posW;
    surface.albedo = baseColor.rgb;
    surface.alpha = baseColor.a;
    surface.depth = mul(float4(surface.position, 1.0f), _View).z;
    surface.normal = normalW;
    surface.interpolatedNormal = pin.normalW;
    surface.metallic = GetMetallic(pin.uv);
    surface.roughness = GetRoughness(pin.uv);
    surface.viewDir = normalize(_EyePosW.xyz - pin.posW);
    surface.shadowFade = GetShadowGlobalFade(surface.depth);
    surface.screenUV = pin.posH.xy * _RenderTargetSize.zw;
        
    float3 radiance = 0.0f;
        
    //direct Light

    radiance += Shading(surface, GetSun(surface));
    
    for (int i = 0; i < GetPointLightCount(); i++)
    {
        radiance += Shading(surface, GetPointLight(i, surface));
    }
    
    for (int j = 0; j < GetSpotLightCount(); j++)
    {
        radiance += Shading(surface, GetSpotLight(j, surface));
    }
    
    // indirect Light by image
    radiance += EnvShading(surface);

    return float4(radiance, 1.0f);
}