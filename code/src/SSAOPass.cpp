#include "stdafx.h"
#include "SSAOPass.h"
#include "util.h"
#include "mesh.h"
#include "globals.h"
#include "frameResource.h"
#include "scene.h"
#include "camera.h"
#include "descriptor.h"
#include "gameApp.h"
using namespace std;
using namespace DirectX;


void SSAOPass::PreparePass(const GraphicContext& context)
{
    CreateResource(context);
    CreateNormalSignaturePSO(context);
    CreateSSAOSignaturePSO(context);
    CreateBlurSignaturePSO(context);
}

void SSAOPass::PreprocessPass(const GraphicContext& context)
{
    SSAOConstant ssaoParam;
    ssaoParam.AORadius = 0.4f;
    ssaoParam.AOSampleCount = 24;
    ssaoParam.AOPower = 6.0f;
    ssaoParam.AOFadeRange = 0.3f;
    ssaoParam.NormalMapIndex = mNormalMap->GetSrvDescriptorData().HeapIndex;
    ssaoParam.DepthMapIndex = GameApp::GetApp()->GetDepthStencilTarget()->GetSrvDescriptorData().HeapIndex;
    mSSAOConstant->CopyData(ssaoParam);

    RenderTargetParamConstant renderTargetParam;
    renderTargetParam.RenderTargetSize = XMFLOAT4(mSSAOWidth, mSSAOHeight, 1.0f / mSSAOWidth, 1.0f / mSSAOHeight);
    mSSAORTConstant->CopyData(renderTargetParam);

    BlurConstant blurParam;
    blurParam.BlurRadius = 5;
    blurParam.RangeSigma = 3.0f;
    blurParam.DepthSigma = 2.0f;
    blurParam.NormalSigma = 1.0f;
    blurParam.NormalMapIndex = ssaoParam.NormalMapIndex;
    blurParam.DepthMapIndex = ssaoParam.DepthMapIndex;
    blurParam.MapWidth = mSSAOWidth;
    blurParam.MapHeight = mSSAOHeight;
    mBlurConstant->CopyData(blurParam);
}

void SSAOPass::DrawPass(const GraphicContext& context)
{
    DrawNormalDepth(context);
    DrawSSAO(context);
    BlurSSAO(context);
}

void SSAOPass::ReleasePass(const GraphicContext& context)
{

}

void SSAOPass::CreateResource(const GraphicContext& context)
{
    float normalClearValue[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    CD3DX12_CLEAR_VALUE normalOptClear(DXGI_FORMAT_R16G16B16A16_FLOAT, normalClearValue);
    mNormalMap = make_unique<RenderTexture>(
        context.device, context.descriptorHeap, context.screenWidth, context.screenHeight,
        1, 1, TextureDimension::Tex2D,
        RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureState::Common, &normalOptClear);

    mSSAOWidth = context.screenWidth / 2;
    mSSAOHeight = context.screenHeight / 2;
    float SSAOClearValue[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    CD3DX12_CLEAR_VALUE SSAOOptClear(DXGI_FORMAT_R16_UNORM, SSAOClearValue);
    mSSAOMap = make_unique<RenderTexture>(
        context.device, context.descriptorHeap, mSSAOWidth, mSSAOHeight,
        1, 1, TextureDimension::Tex2D,
        RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16_UNORM, TextureState::Common, &SSAOOptClear);

    mSSAOBlurXMap = make_unique<UnorderAccessTexture>(
        context.device, context.descriptorHeap, 
        mSSAOWidth, mSSAOHeight,
        DXGI_FORMAT_R16_UNORM);

    size_t SSAOKey = hash<string>()("SSAO");
    mSSAOBlurYMap = Globals::UATextureContainer.Insert(
        SSAOKey,
        make_unique<UnorderAccessTexture>(
            context.device, context.descriptorHeap,
            mSSAOWidth, mSSAOHeight,
            DXGI_FORMAT_R16_UNORM)
    );

    mSSAOConstant = make_unique<UploadBuffer<SSAOConstant, true>>(context.device);
    mSSAORTConstant = make_unique<UploadBuffer<RenderTargetParamConstant, true>>(context.device);
    mBlurConstant = make_unique<UploadBuffer<BlurConstant, true>>(context.device);
}

void SSAOPass::CreateNormalSignaturePSO(const GraphicContext& context)
{
    // create signature
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)NormalRootSignatureParam::COUNT];
    slotRootParameter[(int)NormalRootSignatureParam::ObjectConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)NormalRootSignatureParam::MaterialConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)NormalRootSignatureParam::CameraConstant].InitAsConstantBufferView(2);
    slotRootParameter[(int)NormalRootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init((int)NormalRootSignatureParam::COUNT, slotRootParameter, Globals::StaticSamplers.size(), Globals::StaticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(context.device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mNormalDepthSignature.GetAddressOf())));

    const D3D_SHADER_MACRO macros[] =
    {
#ifdef REVERSE_Z
            "REVERSE_Z", "1",
#endif
        NULL, NULL
    };
    // create pso
    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "NormalDepthPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "NormalDepthPass.hlsl",
        macros, "ps", "ps_5_1");


    D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;
    ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    basePsoDesc.InputLayout = { Vertex::InputLayout.data(), static_cast<UINT>(Vertex::InputLayout.size()) };
    basePsoDesc.pRootSignature = mNormalDepthSignature.Get();
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
    basePsoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = context.depthStencilFormat;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mNormalDepthPSO.GetAddressOf())));
}

void SSAOPass::CreateSSAOSignaturePSO(const GraphicContext& context)
{
    // create signature
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)SSAORootSignatureParam::COUNT];
    slotRootParameter[(int)SSAORootSignatureParam::RenderTargetParamConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)SSAORootSignatureParam::CameraConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)SSAORootSignatureParam::SSAOConstant].InitAsConstantBufferView(2);
    slotRootParameter[(int)SSAORootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init((int)SSAORootSignatureParam::COUNT, slotRootParameter, Globals::StaticSamplers.size(), Globals::StaticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(context.device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mSSAOSignature.GetAddressOf())));

    // create pso
    const D3D_SHADER_MACRO macros[] =
    {
#ifdef REVERSE_Z
            "REVERSE_Z", "1",
#endif
        NULL, NULL
    };
    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "SSAOPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "SSAOPass.hlsl",
        macros, "ps", "ps_5_1");


    D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;
    ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    basePsoDesc.InputLayout = { Vertex::InputLayout.data(), static_cast<UINT>(Vertex::InputLayout.size()) };
    basePsoDesc.pRootSignature = mNormalDepthSignature.Get();
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
    basePsoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mSSAOPSO.GetAddressOf())));
}

void SSAOPass::CreateBlurSignaturePSO(const GraphicContext& context)
{
    // signature
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)BlurRootSignatureParam::COUNT];
    slotRootParameter[(int)BlurRootSignatureParam::BlurConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)BlurRootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable);
    slotRootParameter[(int)BlurRootSignatureParam::AOSrvTable].InitAsDescriptorTable(1, &srvTable);
    slotRootParameter[(int)BlurRootSignatureParam::AOUavTable].InitAsDescriptorTable(1, &uavTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc((int)BlurRootSignatureParam::COUNT, slotRootParameter,
        0, nullptr,
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
        IID_PPV_ARGS(mBlurSignature.GetAddressOf())));

    // PSO
    ComPtr<ID3DBlob> xBlur = CompileShader(Globals::ShaderPath / "SSAOBlurPass.hlsl",
        nullptr, "XBlur_cs", "cs_5_1");

    ComPtr<ID3DBlob> yBlur = CompileShader(Globals::ShaderPath / "SSAOBlurPass.hlsl",
        nullptr, "YBlur_cs", "cs_5_1");

    D3D12_COMPUTE_PIPELINE_STATE_DESC xBlurPSO = {};
    xBlurPSO.pRootSignature = mBlurSignature.Get();
    xBlurPSO.CS =
    {
        reinterpret_cast<BYTE*>(xBlur->GetBufferPointer()),
        xBlur->GetBufferSize()
    };
    xBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(context.device->CreateComputePipelineState(&xBlurPSO, IID_PPV_ARGS(mBlurXPSO.GetAddressOf())));

    D3D12_COMPUTE_PIPELINE_STATE_DESC yBlurPSO = {};
    yBlurPSO.pRootSignature = mBlurSignature.Get();
    yBlurPSO.CS =
    {
        reinterpret_cast<BYTE*>(yBlur->GetBufferPointer()),
        yBlur->GetBufferSize()
    };
    yBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(context.device->CreateComputePipelineState(&yBlurPSO, IID_PPV_ARGS(mBlurYPSO.GetAddressOf())));
}

void SSAOPass::DrawNormalDepth(const GraphicContext& context)
{
    context.commandList->SetGraphicsRootSignature(mNormalDepthSignature.Get());
    context.commandList->SetPipelineState(mNormalDepthPSO.Get());

    RenderTexture* depthTarget = GameApp::GetApp()->GetDepthStencilTarget();
    depthTarget->TransitionTo(context.commandList, TextureState::Write);

    mNormalMap->Clear(context.commandList, 0, 0);
    mNormalMap->TransitionTo(context.commandList, TextureState::Write);
    mNormalMap->SetAsRenderTarget(context.commandList, 0, 0, *depthTarget, 0, 0);

    context.commandList->SetGraphicsRootConstantBufferView((int)NormalRootSignatureParam::CameraConstant,
        context.frameResource->ConstantCamera->GetElementGPUAddress());

    context.commandList->SetGraphicsRootDescriptorTable((int)NormalRootSignatureParam::Texture2DTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

    Camera* camera = context.scene->GetCamera();
    camera->UpdateViewMatrix();
    const vector<Instance*>& renderInstances = context.scene->GetVisibleRenderInstances(camera->GetFrustum(), camera->GetInvView(), camera->GetPosition(), camera->GetLook());

    int currMaterialID = -1;
    for (size_t i = 0; i < renderInstances.size(); i++)
    {
        Instance* instance = renderInstances[i];
        context.commandList->SetGraphicsRootConstantBufferView((int)NormalRootSignatureParam::ObjectConstant, context.frameResource->ConstantObject->GetElementGPUAddress(instance->GetID()));

        // set material if need
        if (currMaterialID != instance->GetMaterial()->GetID())
        {
            currMaterialID = instance->GetMaterial()->GetID();
            context.commandList->SetGraphicsRootConstantBufferView((int)NormalRootSignatureParam::MaterialConstant, context.frameResource->ConstantMaterial->GetElementGPUAddress(currMaterialID));
        }

        instance->GetMesh()->Draw(context.commandList);
    }

    depthTarget->TransitionTo(context.commandList, TextureState::Read);
    mNormalMap->TransitionTo(context.commandList, TextureState::Read);
}

void SSAOPass::DrawSSAO(const GraphicContext& context)
{
    context.commandList->SetGraphicsRootSignature(mSSAOSignature.Get());
    context.commandList->SetPipelineState(mSSAOPSO.Get());

    mSSAOMap->Clear(context.commandList, 0, 0);
    mSSAOMap->TransitionTo(context.commandList, TextureState::Write);
    mSSAOMap->SetAsRenderTarget(context.commandList, 0, 0);

    context.commandList->SetGraphicsRootConstantBufferView((int)SSAORootSignatureParam::RenderTargetParamConstant,
        mSSAORTConstant->GetElementGPUAddress());

    context.commandList->SetGraphicsRootConstantBufferView((int)SSAORootSignatureParam::CameraConstant,
        context.frameResource->ConstantCamera->GetElementGPUAddress());

    context.commandList->SetGraphicsRootConstantBufferView((int)SSAORootSignatureParam::SSAOConstant,
        mSSAOConstant->GetElementGPUAddress());

    context.commandList->SetGraphicsRootDescriptorTable((int)SSAORootSignatureParam::Texture2DTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());


    // Draw fullscreen quad.
    context.commandList->IASetVertexBuffers(0, 0, nullptr);
    context.commandList->IASetIndexBuffer(nullptr);
    context.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context.commandList->DrawInstanced(3, 1, 0, 0);

    mSSAOMap->TransitionTo(context.commandList, TextureState::Read);
}

void SSAOPass::BlurSSAO(const GraphicContext& context)
{
    
    context.commandList->SetComputeRootSignature(mBlurSignature.Get());

    context.commandList->SetComputeRootConstantBufferView(
        (int)BlurRootSignatureParam::BlurConstant,
        mBlurConstant->GetElementGPUAddress()
    );

    context.commandList->SetComputeRootDescriptorTable(
        (int)BlurRootSignatureParam::Texture2DTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart()
    );

    mSSAOBlurYMap->CopyResource(context.commandList, *mSSAOMap);
    for (size_t i = 0; i < mBlurCount; i++)
    {
        // Y to X
        context.commandList->SetPipelineState(mBlurXPSO.Get());
        context.commandList->SetComputeRootDescriptorTable(
            (int)BlurRootSignatureParam::AOSrvTable,
            mSSAOBlurYMap->GetSrvDescriptorData().GPUHandle
        );
        context.commandList->SetComputeRootDescriptorTable(
            (int)BlurRootSignatureParam::AOUavTable,
            mSSAOBlurXMap->GetUavDescriptorData().GPUHandle
        );
        mSSAOBlurYMap->TransitionTo(context.commandList, TextureState::Read);
        mSSAOBlurXMap->TransitionTo(context.commandList, TextureState::Write);
        UINT numGroupX = (UINT)ceilf(float(mSSAOWidth) / COMPUTE_THREAD_GROUP);
        context.commandList->Dispatch(numGroupX, mSSAOHeight, 1);

        // X to Y
        context.commandList->SetPipelineState(mBlurYPSO.Get());
        context.commandList->SetComputeRootDescriptorTable(
            (int)BlurRootSignatureParam::AOSrvTable,
            mSSAOBlurXMap->GetSrvDescriptorData().GPUHandle
        );
        context.commandList->SetComputeRootDescriptorTable(
            (int)BlurRootSignatureParam::AOUavTable,
            mSSAOBlurYMap->GetUavDescriptorData().GPUHandle
        );
        mSSAOBlurXMap->TransitionTo(context.commandList, TextureState::Read);
        mSSAOBlurYMap->TransitionTo(context.commandList, TextureState::Write);
        UINT numGroupY = (UINT)ceilf(float(mSSAOHeight) / COMPUTE_THREAD_GROUP);
        context.commandList->Dispatch(mSSAOWidth, numGroupY, 1);
    }

    mSSAOBlurYMap->TransitionTo(context.commandList, TextureState::Read);
}



