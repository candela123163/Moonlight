#include "stdafx.h"
#include "outputPass.h"
#include "texture.h"
#include "mesh.h"
#include "globals.h"
#include "gameApp.h"
using namespace std;


void OutputPass::PreparePass(const GraphicContext& context)
{
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::InputTexture].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

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
    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "OutputPass.hlsl",
        nullptr, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "OutputPass.hlsl",
        nullptr, "ps", "ps_5_1");


    D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;
    ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    basePsoDesc.InputLayout = { Vertex::InputLayout.data(), static_cast<UINT>(Vertex::InputLayout.size()) };
    basePsoDesc.pRootSignature = mSignature.Get();
    basePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
        vs->GetBufferSize()
    };
    basePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
        ps->GetBufferSize()
    };
    basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    basePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    basePsoDesc.SampleMask = UINT_MAX;
    basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    basePsoDesc.NumRenderTargets = 1;
    basePsoDesc.RTVFormats[0] = context.backBufferFormat;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));
}

void OutputPass::PreprocessPass(const GraphicContext& context)
{

}

void OutputPass::DrawPass(const GraphicContext& context)
{
    ITexture* inputRT = nullptr;
    if (context.renderOption->BloomEnable)
    {
        inputRT = Globals::UATextureContainer.Get(hash<string>()("Bloom"));
    }
    else if(context.renderOption->TAAEnable)
    {
        inputRT = Globals::RenderTextureContainer.Get(hash<string>()("TAAMap"));
    }
    else
    {
        inputRT = Globals::RenderTextureContainer.Get(hash<string>()("OpaqueRT"));
    }
    
    inputRT->TransitionTo(context.commandList, TextureState::Read);
    GameApp::GetApp()->SetDefaultRenderTarget(true, false);
    
    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetPipelineState(mPSO.Get());

    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::InputTexture,
        inputRT->GetSrvDescriptorData().GPUHandle);

    context.commandList->IASetVertexBuffers(0, 0, nullptr);
    context.commandList->IASetIndexBuffer(nullptr);
    context.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.commandList->DrawInstanced(3, 1, 0, 0);
}

void OutputPass::ReleasePass(const GraphicContext& context)
{
}