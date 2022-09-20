#include "stdafx.h"
#include "temporalAAPass.h"
#include "texture.h"
#include "gameApp.h"
using namespace std;
using namespace DirectX;

void TAAPass::PreparePass(const GraphicContext& context)
{
    // create resources
    mHistory = make_unique<RenderTexture>(
        context.device, context.descriptorHeap,
        context.screenWidth, context.screenHeight,
        1, 1, TextureDimension::Tex2D,
        RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16G16B16A16_FLOAT);

    mTAAOut = Globals::RenderTextureContainer.Insert(
        hash<string>()("TAAMap"),
        make_unique<RenderTexture>(
            context.device, context.descriptorHeap,
            context.screenWidth, context.screenHeight,
            1, 1, TextureDimension::Tex2D,
            RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16G16B16A16_FLOAT)
    );

    // root signature
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);
    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::CameraConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::TAAConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)RootSignatureParam::Texture2D].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

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

    // pso
    const D3D_SHADER_MACRO macros[] =
    {
#ifdef REVERSE_Z
        "REVERSE_Z", "1",
#endif
        NULL, NULL
    };

    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "TAAPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "TAAPass.hlsl",
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
    basePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    basePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    basePsoDesc.SampleMask = UINT_MAX;
    basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    basePsoDesc.NumRenderTargets = 1;
    basePsoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));
}

void TAAPass::PreprocessPass(const GraphicContext& context)
{
    RenderTexture* frameColor = Globals::RenderTextureContainer.Get(hash<string>()("OpaqueRT"));
    RenderTexture* frameDepth = GameApp::GetApp()->GetDepthStencilTarget();
    RenderTexture* motion = Globals::RenderTextureContainer.Get(hash<string>()("MotionMap"));

    mTAAParam.TexelSize = XMFLOAT2(1.0 / context.screenWidth, 1.0 / context.screenHeight);
    mTAAParam.FrameColorMapIndex = frameColor->GetSrvDescriptorData().HeapIndex;
    mTAAParam.FrameDepthMapIndex = frameDepth->GetSrvDescriptorData().HeapIndex;
    mTAAParam.MotionMapIndex = motion->GetSrvDescriptorData().HeapIndex;
    mTAAParam.HistoryColorMapIndex = mHistory->GetSrvDescriptorData().HeapIndex;
    mTAAParam.HistoryClipBound = XMFLOAT2(1.25f, 6.0f);
    mTAAParam.MotionWeightFactor = 3000.0f;
    mTAAParam.Sharpness = 0.02f;

    context.renderOption->TAAEnable = true;
    context.renderOption->TAAClipBound = mTAAParam.HistoryClipBound;
    context.renderOption->TAAMotionWeight = mTAAParam.MotionWeightFactor;
    context.renderOption->TAASharpness = mTAAParam.Sharpness;
}

void TAAPass::DrawPass(const GraphicContext& context)
{
    if (!context.renderOption->TAAEnable)
    {
        return;
    }

    if (context.frameCount == 0)
    {
        RenderTexture* frameColor = Globals::RenderTextureContainer.Get(hash<string>()("OpaqueRT"));
        mHistory->CopyResource(context.commandList, *frameColor);
    }
    
    mHistory->TransitionTo(context.commandList, TextureState::Read);
    mTAAOut->Clear(context.commandList, 0, 0);
    mTAAOut->TransitionTo(context.commandList, TextureState::Write);
    mTAAOut->SetAsRenderTarget(context.commandList, 0, 0);

    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetPipelineState(mPSO.Get());

    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::Texture2D,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::CameraConstant,
        context.frameResource->ConstantCamera->GetElementGPUAddress());

    mTAAParam.HistoryClipBound = context.renderOption->TAAClipBound;
    mTAAParam.MotionWeightFactor = context.renderOption->TAAMotionWeight;
    mTAAParam.Sharpness = context.renderOption->TAASharpness;

    TAAPassData* passData = dynamic_cast<TAAPassData*>(context.frameResource->GetOrCreate(this,
        [&]() {
        return make_unique<TAAPassData>(context.device);
    }));
    passData->ssaoConstant->CopyData(mTAAParam);

    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::TAAConstant,
        passData->ssaoConstant->GetElementGPUAddress());
    
    context.commandList->IASetVertexBuffers(0, 0, nullptr);
    context.commandList->IASetIndexBuffer(nullptr);
    context.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.commandList->DrawInstanced(3, 1, 0, 0);

    mHistory->CopyResource(context.commandList, *mTAAOut);
}

void TAAPass::ReleasePass(const GraphicContext& context)
{

}