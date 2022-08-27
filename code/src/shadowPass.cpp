#include "stdafx.h"
#include "shadowPass.h"
#include "globals.h"
#include "mesh.h"
#include "scene.h"
#include "light.h"
using namespace DirectX;

void ShadowPass::PreparePass(const GraphicContext& context)
{
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::ObjectConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::ShadowCasterConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)RootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

    // create root signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init((int)RootSignatureParam::COUNT, slotRootParameter, Globals::StaticSamplers.size(), Globals::StaticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(context.device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mSignature.GetAddressOf())));

    // create pso
    ComPtr<ID3DBlob> vs1 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        nullptr, "vs_shadow", "vs_5_1");
    ComPtr<ID3DBlob> ps1 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        nullptr, "ps_shadow", "ps_5_1");

    ComPtr<ID3DBlob> vs2 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        nullptr, "vs_point_shadow", "vs_5_1");
    ComPtr<ID3DBlob> ps2 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        nullptr, "ps_point_shadow", "ps_5_1");

    // directional & spot shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc;
    ZeroMemory(&shadowPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    shadowPsoDesc.InputLayout = { Vertex::InputLayout.data(), static_cast<UINT>(Vertex::InputLayout.size()) };
    shadowPsoDesc.pRootSignature = mSignature.Get();
    shadowPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs1->GetBufferPointer()),
        vs1->GetBufferSize()
    };
    shadowPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps1->GetBufferPointer()),
        ps1->GetBufferSize()
    };
    shadowPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    shadowPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    shadowPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    shadowPsoDesc.SampleMask = UINT_MAX;
    shadowPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    shadowPsoDesc.NumRenderTargets = 1;
    shadowPsoDesc.RTVFormats[0] = context.backBufferFormat;
    shadowPsoDesc.SampleDesc.Count = 1;
    shadowPsoDesc.SampleDesc.Quality = 0;
    shadowPsoDesc.DSVFormat = context.depthStencilFormat;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(mShadowPSO.GetAddressOf())));

    // point shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pointShadowPsoDesc = shadowPsoDesc;
    shadowPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs2->GetBufferPointer()),
        vs2->GetBufferSize()
    };
    shadowPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps2->GetBufferPointer()),
        ps2->GetBufferSize()
    };

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(mPointShadowPSO.GetAddressOf())));
}

void ShadowPass::PreprocessPass(const GraphicContext& context)
{

}

void ShadowPass::DrawPass(const GraphicContext& context)
{

}

void ShadowPass::ReleasePass(const GraphicContext& context)
{

}

void ShadowPass::DrawSunShadow(const GraphicContext& context)
{

}

void ShadowPass::DrawSpotLightShadow(const GraphicContext& context)
{
    /*const auto& spotLights = context.scene->GetSpotLights();
    XMVECTOR up = XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0f, 1.0f, 0.0f)));
    
    UINT spotConstantOffset = MAX_CASCADE_COUNT;
    for (size_t i = 0; i < spotLights.size(); i++)
    {
        ShadowCasterConstant pointShadowConstant;
        const auto& spotLight = spotLights[i];

        XMMATRIX view = XMMatrixLookToLH(
            XMLoadFloat3(&spotLight.Position),
            XMLoadFloat3(&spotLight.Direction),
            up
        );
        XMMATRIX project = XMMatrixPerspectiveFovLH(
            spotLight.OutterAngle, 1.0f, spotLight.Near, spotLight.Range
        );
        XMMATRIX viewProject = XMMatrixTranspose(XMMatrixMultiply(view, project));
        
        XMStoreFloat4x4(&pointShadowConstant.LightViewProject, viewProject);
        pointShadowConstant.LightPosition = XMFLOAT4(
            spotLight.Position.x,
            spotLight.Position.y,
            spotLight.Position.z,
            1.0f / spotLight.Range
        );

        context.frameResource->ConstantShadowCaster->CopyData(pointShadowConstant, spotConstantOffset + i);
    }*/
}

void ShadowPass::DrawPointLightShadow(const GraphicContext& context)
{

}