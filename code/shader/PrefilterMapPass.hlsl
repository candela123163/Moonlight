#include "StaticSamplers.hlsli"
#include "Util.hlsli"
#include "BRDF.hlsli"

TextureCube _EnvMap : register(t0);

cbuffer IBLConstant : register(b0)
{
    float4x4 _ViewProject;
}

cbuffer RoughnessConstant : register(b1)
{
    float _Roughness;
    float _Resolution;
}

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posW : VAR_POSITION_L;
};


VertexOut vs(float3 posL : POSITION)
{
    VertexOut vout;
    vout.posH = CubeMapVertexTransform(posL, _ViewProject, 0.0f);
    vout.posW = posL;
    return vout;
}

#define SAMPLE_COUNT 1024

float4 ps(VertexOut pin) : SV_TARGET
{    
    // make the simplyfying assumption that V equals R equals the normal 
    float3 N = normalize(pin.posW);
    float3 R = N;
    float3 V = R;
    
    float3 prefilteredColor = 0.0f;
    float totalWeight = 0.0f;
    
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 hLocal = GGX_Sample(Xi, _Roughness);
        float3 hWorld = NormalTangentToWorld(hLocal, N);
        float3 L = reflect(-V, hWorld);
        
        float NdotL = saturate(dot(N, L));
        
        if (NdotL > 0.0f)
        {            
            // weight by dot(N, L)
            prefilteredColor += _EnvMap.Sample(_SamplerLinearClamp, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefilteredColor /= totalWeight;
    
    return float4(prefilteredColor, 1.0f);
}