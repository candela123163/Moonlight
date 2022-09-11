#include "stdafx.h"
#include "FXAAPass.h"
#include "texture.h"
using namespace std;
using namespace DirectX;

void FXAAPass::PreparePass(const GraphicContext& context)
{
    CreateResource(context);
    CreateSignaturePSO(context);
}

void FXAAPass::PreprocessPass(const GraphicContext& context)
{
    size_t rtKey = hash<string>()("Bloom");
    mInputRT = Globals::UATextureContainer.Get(rtKey);

    mFXAAParam.FixedThreshold = 0.0433f;
    mFXAAParam.RelativeThreshold = 0.86f;
    mFXAAParam.SubPixelBlending = 0.75f;
    mFXAAParam.TexelSize = XMFLOAT2(1.0f / context.screenWidth, 1.0f / context.screenHeight);
    mFXAAConstant->CopyData(mFXAAParam);
}

void FXAAPass::DrawPass(const GraphicContext& context)
{
    context.commandList->SetComputeRootSignature(mSignature.Get());
    context.commandList->SetPipelineState(mPSO.Get());

    context.commandList->SetComputeRootConstantBufferView(
        (int)RootSignatureParam::FXAAConstant,
        mFXAAConstant->GetElementGPUAddress()
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::InputTexture,
        mInputRT->GetSrvDescriptorData().GPUHandle
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::OutputTexture,
        mFXAAOutMap->GetUavDescriptorData(0).GPUHandle
    );

    mInputRT->TransitionTo(context.commandList, TextureState::Read);
    mFXAAOutMap->TransitionTo(context.commandList, TextureState::Write);

    UINT numGroupX = (UINT)ceilf(float(mFXAAOutMap->GetWidth()) / COMPUTE_THREAD_GROUP);
    context.commandList->Dispatch(numGroupX, mFXAAOutMap->GetHeight(), 1);
}

void FXAAPass::ReleasePass(const GraphicContext& context)
{


}

void FXAAPass::CreateResource(const GraphicContext& context)
{
    size_t rtKey = hash<string>()("FXAA");
    mFXAAOutMap = Globals::UATextureContainer.Insert(
        rtKey,
        make_unique<UnorderAccessTexture>(
            context.device, context.descriptorHeap,
            context.screenWidth, context.screenHeight,
            1,
            DXGI_FORMAT_R8G8B8A8_UNORM)
    );

    mFXAAConstant = make_unique<UploadBuffer<FXAAConstant, true>>(context.device);
}

void FXAAPass::CreateSignaturePSO(const GraphicContext& context)
{
    CD3DX12_DESCRIPTOR_RANGE inputSrvTable;
    inputSrvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::FXAAConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::InputTexture].InitAsDescriptorTable(1, &inputSrvTable);
    slotRootParameter[(int)RootSignatureParam::OutputTexture].InitAsDescriptorTable(1, &uavTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((int)RootSignatureParam::COUNT, slotRootParameter,
        Globals::StaticSamplers.size(), Globals::StaticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(context.device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mSignature.GetAddressOf())));

    // PSO
    ComPtr<ID3DBlob> fxaa_cs = CompileShader(Globals::ShaderPath / "FXAAPass.hlsl",
        nullptr, "FXAA_cs", "cs_5_1");

    D3D12_COMPUTE_PIPELINE_STATE_DESC fxaaPSO = {};
    fxaaPSO.pRootSignature = mSignature.Get();
    fxaaPSO.CS =
    {
        reinterpret_cast<BYTE*>(fxaa_cs->GetBufferPointer()),
        fxaa_cs->GetBufferSize()
    };
    fxaaPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(context.device->CreateComputePipelineState(&fxaaPSO, IID_PPV_ARGS(mPSO.GetAddressOf())));
}