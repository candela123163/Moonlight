#include "stdafx.h"
#include "bloomPass.h"
#include "texture.h"
using namespace std;
using namespace DirectX;

void BloomPass::PreparePass(const GraphicContext& context)
{
    CalculateBloomParam(context.screenWidth, context.screenHeight);
    CreateResource(context);
    CreateSignaturePSO(context);
}

void BloomPass::PreprocessPass(const GraphicContext& context)
{
    size_t rtKey = hash<string>()("TAAMap");
    mInputRT = Globals::RenderTextureContainer.Get(rtKey);

    mBloomConstant->CopyData(mBloomParam);
}

void BloomPass::DrawPass(const GraphicContext& context)
{
    context.commandList->SetComputeRootSignature(mSignature.Get());

    DoPrefilter(context);
    DoDownSample(context);
    DoUpSample(context);
    DoCombine(context);
}

void BloomPass::ReleasePass(const GraphicContext& context)
{

}

void BloomPass::CreateResource(const GraphicContext& context)
{
    mDownSampleChain = make_unique<UnorderAccessTexture>(
        context.device, context.descriptorHeap,
        context.screenWidth * 0.5f, context.screenHeight * 0.5f, 
        mIterationCount,
        DXGI_FORMAT_R16G16B16A16_FLOAT);

    size_t bloomKey = hash<string>()("Bloom");
    mUpSampleChain = Globals::UATextureContainer.Insert(
        bloomKey,
        make_unique<UnorderAccessTexture>(
            context.device, context.descriptorHeap,
            context.screenWidth, context.screenHeight,
            mIterationCount,
            DXGI_FORMAT_R16G16B16A16_FLOAT)
    );

    mBloomConstant = make_unique<UploadBuffer<BloomConstant, true>>(context.device);
}

void BloomPass::CreateSignaturePSO(const GraphicContext& context)
{
    // signature
    CD3DX12_DESCRIPTOR_RANGE bloomChainSrvTable;
    bloomChainSrvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE inputSrvTable;
    inputSrvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::BloomConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::MipLevelConstant32].InitAsConstants(mNumConst32, 1);
    slotRootParameter[(int)RootSignatureParam::BloomChainTexture].InitAsDescriptorTable(1, &bloomChainSrvTable);
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
    ComPtr<ID3DBlob> prefilter_cs = CompileShader(Globals::ShaderPath / "BloomPass.hlsl",
        nullptr, "Prefiler_cs", "cs_5_1");

    D3D12_COMPUTE_PIPELINE_STATE_DESC prefilterPSO = {};
    prefilterPSO.pRootSignature = mSignature.Get();
    prefilterPSO.CS =
    {
        reinterpret_cast<BYTE*>(prefilter_cs->GetBufferPointer()),
        prefilter_cs->GetBufferSize()
    };
    prefilterPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(context.device->CreateComputePipelineState(&prefilterPSO, IID_PPV_ARGS(mPrefilterPSO.GetAddressOf())));


    ComPtr<ID3DBlob> downSample_cs = CompileShader(Globals::ShaderPath / "BloomPass.hlsl",
        nullptr, "DownSample_cs", "cs_5_1");

    D3D12_COMPUTE_PIPELINE_STATE_DESC downSamplePSO = prefilterPSO;
    downSamplePSO.CS =
    {
        reinterpret_cast<BYTE*>(downSample_cs->GetBufferPointer()),
        downSample_cs->GetBufferSize()
    };
    ThrowIfFailed(context.device->CreateComputePipelineState(&downSamplePSO, IID_PPV_ARGS(mDownSamplePSO.GetAddressOf())));


    ComPtr<ID3DBlob> upSample_cs = CompileShader(Globals::ShaderPath / "BloomPass.hlsl",
        nullptr, "UpSample_cs", "cs_5_1");

    D3D12_COMPUTE_PIPELINE_STATE_DESC upSamplePSO = prefilterPSO;
    upSamplePSO.CS =
    {
        reinterpret_cast<BYTE*>(upSample_cs->GetBufferPointer()),
        upSample_cs->GetBufferSize()
    };
    ThrowIfFailed(context.device->CreateComputePipelineState(&upSamplePSO, IID_PPV_ARGS(mUpSamplePSO.GetAddressOf())));


    ComPtr<ID3DBlob> combine_cs = CompileShader(Globals::ShaderPath / "BloomPass.hlsl",
        nullptr, "Combine_cs", "cs_5_1");

    D3D12_COMPUTE_PIPELINE_STATE_DESC combinePSO = prefilterPSO;
    combinePSO.CS =
    {
        reinterpret_cast<BYTE*>(combine_cs->GetBufferPointer()),
        combine_cs->GetBufferSize()
    };
    ThrowIfFailed(context.device->CreateComputePipelineState(&combinePSO, IID_PPV_ARGS(mCombinePSO.GetAddressOf())));
}

void BloomPass::DoPrefilter(const GraphicContext& context)
{
    context.commandList->SetPipelineState(mPrefilterPSO.Get());

    context.commandList->SetComputeRootConstantBufferView(
        (int)RootSignatureParam::BloomConstant,
        mBloomConstant->GetElementGPUAddress()
    );

    int bloomChainMipLevel = 0;
    int inputMipLevel = 0;
    int mipLevels[mNumConst32] = {
        bloomChainMipLevel,
        inputMipLevel
    };

    context.commandList->SetComputeRoot32BitConstants(
        (int)RootSignatureParam::MipLevelConstant32,
        mNumConst32,
        &mipLevels,
        0
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::InputTexture,
        mInputRT->GetSrvDescriptorData().GPUHandle
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::OutputTexture,
        mDownSampleChain->GetUavDescriptorData(0).GPUHandle
    );

    mDownSampleChain->TransitionSubResourceTo(context.commandList, 0, 0, TextureState::Write);
    mInputRT->TransitionTo(context.commandList, TextureState::Read);

    UINT numGroupX = (UINT)ceilf(float(mDownSampleChain->GetWidth(0)) / COMPUTE_THREAD_GROUP);
    context.commandList->Dispatch(numGroupX, mDownSampleChain->GetHeight(0), 1);

}

void BloomPass::DoDownSample(const GraphicContext& context)
{
    context.commandList->SetPipelineState(mDownSamplePSO.Get());

    for (size_t iter = 1; iter < mIterationCount; iter++)
    {
        int bloomChainMipLevel = iter - 1;
        int mipLevels[mNumConst32] = {
            bloomChainMipLevel,
            0
        };

        context.commandList->SetComputeRoot32BitConstants(
            (int)RootSignatureParam::MipLevelConstant32,
            mNumConst32,
            &mipLevels,
            0
        );

        context.commandList->SetComputeRootDescriptorTable(
            (int)RootSignatureParam::BloomChainTexture,
            mDownSampleChain->GetSrvDescriptorData().GPUHandle
        );

        context.commandList->SetComputeRootDescriptorTable(
            (int)RootSignatureParam::OutputTexture,
            mDownSampleChain->GetUavDescriptorData(iter).GPUHandle
        );

        mDownSampleChain->TransitionSubResourceTo(context.commandList, 0, bloomChainMipLevel, TextureState::Read);
        mDownSampleChain->TransitionSubResourceTo(context.commandList, 0, iter, TextureState::Write);

        UINT numGroupX = (UINT)ceilf(float(mDownSampleChain->GetWidth(iter)) / COMPUTE_THREAD_GROUP);
        context.commandList->Dispatch(numGroupX, mDownSampleChain->GetHeight(iter), 1);
    }

}

void BloomPass::DoUpSample(const GraphicContext& context)
{
    context.commandList->SetPipelineState(mUpSamplePSO.Get());

    for (size_t iter = mIterationCount - 1; iter > 0; iter--)
    {
        int bloomChainMipLevel;
        int inputMipLevel = iter - 1;
        if (iter == mIterationCount - 1)
        {
            bloomChainMipLevel = iter;
            context.commandList->SetComputeRootDescriptorTable(
                (int)RootSignatureParam::BloomChainTexture,
                mDownSampleChain->GetSrvDescriptorData().GPUHandle
            );
            mDownSampleChain->TransitionSubResourceTo(context.commandList, 0, bloomChainMipLevel, TextureState::Read);
        }
        else
        {
            bloomChainMipLevel = iter + 1;
            context.commandList->SetComputeRootDescriptorTable(
                (int)RootSignatureParam::BloomChainTexture,
                mUpSampleChain->GetSrvDescriptorData().GPUHandle
            );
            mUpSampleChain->TransitionSubResourceTo(context.commandList, 0, bloomChainMipLevel, TextureState::Read);
        }

        int mipLevels[mNumConst32] = {
            bloomChainMipLevel,
            inputMipLevel
        };

        context.commandList->SetComputeRoot32BitConstants(
            (int)RootSignatureParam::MipLevelConstant32,
            mNumConst32,
            &mipLevels,
            0
        );

        context.commandList->SetComputeRootDescriptorTable(
            (int)RootSignatureParam::InputTexture,
            mDownSampleChain->GetSrvDescriptorData().GPUHandle
        );

        context.commandList->SetComputeRootDescriptorTable(
            (int)RootSignatureParam::OutputTexture,
            mUpSampleChain->GetUavDescriptorData(iter).GPUHandle
        );

        mDownSampleChain->TransitionSubResourceTo(context.commandList, 0, inputMipLevel, TextureState::Read);
        mUpSampleChain->TransitionSubResourceTo(context.commandList, 0, iter, TextureState::Write);

        UINT numGroupX = (UINT)ceilf(float(mUpSampleChain->GetWidth(iter)) / COMPUTE_THREAD_GROUP);
        context.commandList->Dispatch(numGroupX, mUpSampleChain->GetHeight(iter), 1);
    }
}

void BloomPass::DoCombine(const GraphicContext& context)
{
    context.commandList->SetPipelineState(mCombinePSO.Get());
    
    int bloomChainMipLevel = 1;
    int inputMipLevel = 0;
    int mipLevels[mNumConst32] = {
        bloomChainMipLevel,
        inputMipLevel
    };

    context.commandList->SetComputeRoot32BitConstants(
        (int)RootSignatureParam::MipLevelConstant32,
        mNumConst32,
        &mipLevels,
        0
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::BloomChainTexture,
        mUpSampleChain->GetSrvDescriptorData().GPUHandle
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::InputTexture,
        mInputRT->GetSrvDescriptorData().GPUHandle
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)RootSignatureParam::OutputTexture,
        mUpSampleChain->GetUavDescriptorData(0).GPUHandle
    );

    mInputRT->TransitionSubResourceTo(context.commandList, 0, 0, TextureState::Read);
    mUpSampleChain->TransitionSubResourceTo(context.commandList, 0, bloomChainMipLevel, TextureState::Read);
    mUpSampleChain->TransitionSubResourceTo(context.commandList, 0, 0, TextureState::Write);

    UINT numGroupX = (UINT)ceilf(float(mUpSampleChain->GetWidth(0)) / COMPUTE_THREAD_GROUP);
    context.commandList->Dispatch(numGroupX, mUpSampleChain->GetHeight(0), 1);

}

void  BloomPass::CalculateBloomParam(float inputWidth, float inputHeight)
{
    float size = min(inputWidth, inputHeight);
    mIterationCount = (int)floorf(log2f(size / mMinDownScaleSize));
    mIterationCount = clamp(mIterationCount, mMinChainLength, mMaxChainLength);

    mBloomParam.Intensity = mIntensity;
    mBloomParam.Threshold = XMFLOAT4(
        mThreshold,
        mThreshold - mThreshold * mKnee,
        2.0f * mThreshold * mKnee,
        0.25f / (mThreshold * mKnee)
    );
}