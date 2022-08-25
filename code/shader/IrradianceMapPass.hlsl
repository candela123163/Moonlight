#include "StaticSamplers.hlsli"
#include "Util.hlsli"

TextureCube _EnvMap : register(t0);

cbuffer IBLConstant : register(b0)
{
    float4x4 _ViewProject;
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

#define SAMPLE_COUNT 2048
float4 ps(VertexOut pin) : SV_TARGET
{
    float3 N = normalize(pin.posW);
    float3 irradiance = 0.0f;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 lightLocal = SampleCosOnHemiSphere(Xi);
        float3 lightWorld = NormalTangentToWorld(lightLocal, N);
        irradiance += _EnvMap.Sample(_SamplerLinearClamp, lightWorld).rgb;
    }
    irradiance /= float(SAMPLE_COUNT);
        
    return float4(irradiance, 1.0f);
}