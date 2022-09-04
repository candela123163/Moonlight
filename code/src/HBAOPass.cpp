#include "stdafx.h"
#include "HBAOPass.h"
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


void HBAOPass::PreparePass(const GraphicContext& context)
{
    CreateResource(context);
    CreateNormalSignaturePSO(context);
    CreateHBAOSignaturePSO(context);
    CreateBlurSignaturePSO(context);
}

void HBAOPass::PreprocessPass(const GraphicContext& context)
{

}

void HBAOPass::DrawPass(const GraphicContext& context)
{
    DrawNormalDepth(context);
}

void HBAOPass::ReleasePass(const GraphicContext& context)
{

}

void HBAOPass::CreateResource(const GraphicContext& context)
{
    float normalClearValue[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    CD3DX12_CLEAR_VALUE normalOptClear(DXGI_FORMAT_R16G16B16A16_FLOAT, normalClearValue);
    mNormalMap = make_unique<RenderTexture>(
        context.device, context.descriptorHeap, context.screenWidth, context.screenHeight,
        1, 1, TextureDimension::Tex2D,
        RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16G16B16A16_FLOAT, TextureState::Common, &normalOptClear);

    UINT HBAOMapWidth = context.screenWidth;
    UINT HBAOMapHeight = context.screenHeight;
    float hbaoClearValue[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    CD3DX12_CLEAR_VALUE hbaoOptClear(DXGI_FORMAT_R16_UNORM, hbaoClearValue);
    mHBAOMap = make_unique<RenderTexture>(
        context.device, context.descriptorHeap, HBAOMapWidth, HBAOMapHeight,
        1, 1, TextureDimension::Tex2D,
        RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16_UNORM, TextureState::Common, &hbaoOptClear);

    mHBAOBlurXMap = make_unique<UnorderAccessTexture>(
        context.device, context.descriptorHeap, 
        HBAOMapWidth, HBAOMapHeight,
        DXGI_FORMAT_R16_UNORM);

    size_t HBAOKey = hash<string>()("HBAO");
    mHBAOBlurYMap = Globals::UATextureContainer.Insert(
        HBAOKey,
        make_unique<UnorderAccessTexture>(
            context.device, context.descriptorHeap,
            HBAOMapWidth, HBAOMapHeight,
            DXGI_FORMAT_R16_UNORM)
    );
}

void HBAOPass::CreateNormalSignaturePSO(const GraphicContext& context)
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

    // create pso
    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "NormalDepthPass.hlsl",
        nullptr, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "NormalDepthPass.hlsl",
        nullptr, "ps", "ps_5_1");


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
    basePsoDesc.RTVFormats[0] = context.backBufferFormat;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = context.depthStencilFormat;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mNormalDepthPSO.GetAddressOf())));
}

void HBAOPass::CreateHBAOSignaturePSO(const GraphicContext& context)
{

}

void HBAOPass::CreateBlurSignaturePSO(const GraphicContext& context)
{
 
}

void HBAOPass::DrawNormalDepth(const GraphicContext& context)
{
    //GameApp::GetApp()->SetDefaultRenderTarget();
    D3D12_CPU_DESCRIPTOR_HANDLE screenDepth = GameApp::GetApp()->DepthStencilView();
    mNormalMap->Clear(context.commandList, 0, 0);
    mNormalMap->TransitionTo(context.commandList, TextureState::Write);
    mNormalMap->SetAsRenderTarget(context.commandList, 0, 0, 0, nullptr, &screenDepth);

    context.commandList->SetGraphicsRootSignature(mNormalDepthSignature.Get());
    context.commandList->SetPipelineState(mNormalDepthPSO.Get());

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

    mNormalMap->TransitionTo(context.commandList, TextureState::Read);
}

void HBAOPass::DrawHBAO(const GraphicContext& context)
{

}

void HBAOPass::BlurHBAO(const GraphicContext& context)
{

}

