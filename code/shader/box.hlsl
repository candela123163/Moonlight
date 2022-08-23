#include "LitInput.hlsli"
#include "Util.hlsli"
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
    float depthV : VAR_DEPTH_V;
};

// simple vertex shader
VertexOut vs(VertexIn vin) 
{
    VertexOut output;
    float4 posW = mul(float4(vin.posL, 1.0f), _World);
    output.posH = mul(posW, _ViewProj);
    output.posW = posW.xyz;
    output.normalW = mul(vin.normalL, (float3x3) _InvTransposeWorld);
    output.tangentW = mul(vin.tangentL, (float3x3)_World);
    output.biTangentW = mul(vin.biTangentL, (float3x3)_World);
    output.uv = vin.uv;
    output.depthV = mul(posW, _View).z;
    
    return output;
}

// simple pixel shader
float4 ps(VertexOut pin) : SV_TARGET
{
    float3 a = 0.0f;
    //float3 l = GetSun().color;
    return float4(GetSun().color, 1.0f);
    // normalize vectors
    //pin.normalW = normalize(pin.normalW);
    //pin.tangentW = normalize(pin.tangentW);
    //pin.biTangentW = normalize(pin.biTangentW);
    
    //float4 baseColor = GetBaseColor(pin.uv);
    
    //clip(baseColor.a - 0.5f);
    
    //float3 normalW;
    //if (NormalMapped())
    //{
    //    normalW = NormalTangentToWorld(GetNormalTS(pin.uv), pin.tangentW, pin.biTangentW, pin.normalW);
    //}
    //else
    //{
    //    normalW = pin.normalW;
    //}
    //normalW = normalize(normalW);

    //Surface surface;
    //surface.position = pin.posW;
    //surface.albedo = baseColor.rgb;
    //surface.alpha = baseColor.a;
    //surface.depth = pin.depthV;
    //surface.normal = normalW;
    //surface.interpolatedNormal = pin.normalW;
    //surface.metallic = GetMetallic(pin.uv);
    //surface.roughness = GetRoughness(pin.uv);
    //surface.viewDir = normalize(pin.posW - _EyePosW.xyz);
    
    //float3 radiance = 0.0f;
    
    //Light light = GetSun();
    //BRDF brdf = DirectBRDF(surface, light.direction);
    //float3 l = IncomingLight(surface, light);
    //radiance += light.color;
    //radiance += IncomingLight(surface, light) * (brdf.diffuse + brdf.specular);
   
    
    //for (int i = 0; i < GetPointLightCount(); i++)
    //{
    //    radiance += Shading(surface, GetPointLight(i, surface));
    //}
    
    //for (int j = 0; j < GetSpotLightCount(); j++)
    //{
    //    radiance += Shading(surface, GetSpotLight(j, surface));
    //}

    return float4(a, 1.0f);
}