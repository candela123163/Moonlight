#include "stdafx.h"
#include "debugPass.h"
#include "util.h"
#include "mesh.h"
#include "globals.h"
#include "frameResource.h"
#include "scene.h"
#include "camera.h"
#include "descriptor.h"
#include "gameApp.h"
using namespace DirectX;
using namespace std;

void DebugPass::PreparePass(const GraphicContext& context)
{
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE texcubeTable;
    texcubeTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 1);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::Constant32Float].InitAsConstants(2, 0);
    slotRootParameter[(int)RootSignatureParam::Constant32UInt].InitAsConstants(10, 1);
    slotRootParameter[(int)RootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[(int)RootSignatureParam::TextureCubeTable].InitAsDescriptorTable(1, &texcubeTable, D3D12_SHADER_VISIBILITY_PIXEL);

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
    const D3D_SHADER_MACRO macros[] =
    {
#ifdef REVERSE_Z
        "REVERSE_Z", "1",
#endif
        NULL, NULL
    };

    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "DebugPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "DebugPass.hlsl",
        macros, "ps", "ps_5_1");


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
#ifdef REVERSE_Z
    basePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
#endif
    basePsoDesc.SampleMask = UINT_MAX;
    basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    basePsoDesc.NumRenderTargets = 1;
    basePsoDesc.RTVFormats[0] = context.backBufferFormat;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = context.depthStencilFormat;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));

    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = 500.0f;
    mScreenViewport.Height = 500.0f;
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, 500, 500};
}


void DebugPass::DrawPass(const GraphicContext& context)
{
    GameApp::GetApp()->SetDefaultRenderTarget();
    context.commandList->RSSetViewports(1, &mScreenViewport);
    context.commandList->RSSetScissorRects(1, &mScissorRect);

    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetPipelineState(mPSO.Get());

    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::Texture2DTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::TextureCubeTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

    float resolution[2] = { mScreenViewport.Width, mScreenViewport.Height };
    context.commandList->SetGraphicsRoot32BitConstants((int)RootSignatureParam::Constant32Float, 2, &resolution, 0);

    const auto& sun = context.scene->GetDirectionalLight();
    int cascadeCount = context.scene->GetCamera()->GetCascadeCount();

    UINT mapIndices[10];
    
    
    for (size_t i = 0; i < 10; i++)
    {
        mapIndices[i] = 0;
        if (i < cascadeCount) {
            mapIndices[i] = sun.ShadowMaps[i]->GetSrvDescriptorData().HeapIndex;
        }
        
    }
    context.commandList->SetGraphicsRoot32BitConstants((int)RootSignatureParam::Constant32UInt, 10, &mapIndices, 0);

    // Draw fullscreen quad.
    context.commandList->IASetVertexBuffers(0, 0, nullptr);
    context.commandList->IASetIndexBuffer(nullptr);
    context.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.commandList->DrawInstanced(6, 1, 0, 0);
}

void DebugPass::ReleasePass(const GraphicContext& context)
{

}

void DebugPass::PreprocessPass(const GraphicContext& context)
{

}