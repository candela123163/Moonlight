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
    // Solid angle associated to a texel of the cubemap
    float saTexel = FOUR_PI / (6.0f * _Resolution * _Resolution);
    
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
            float D = GGX_D(N, hWorld, _Roughness);
            float NdotH = saturate(dot(N, hWorld));
            float HdotV = saturate(dot(hWorld, V));
            float pdf = D * NdotH / (4.0f * HdotV + 0.0001f);
            
            // choose envmap mipLevel accroding sample pdf
            // this code is from
            // https://learnopengl.com/PBR/IBL/Specular-IBL
            float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel = 0.5f * log2(saSample / saTexel);
            
            // weight by dot(N, L)
            prefilteredColor += _EnvMap.SampleLevel(_SamplerLinearClamp, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefilteredColor /= totalWeight;
    
    return float4(prefilteredColor, 1.0f);
}