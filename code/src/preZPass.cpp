#include "stdafx.h"
#include "preZPass.h"
#include "texture.h"
#include "mesh.h"
#include "gameApp.h"
using namespace std;
using namespace DirectX;

void PreZPass::PreparePass(const GraphicContext& context)
{
    size_t normalMapKey = hash<string>()("NormalMap");
    float normalClearValue[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    CD3DX12_CLEAR_VALUE normalOptClear(DXGI_FORMAT_R8G8B8A8_UNORM, normalClearValue);
    mNormalMap = Globals::RenderTextureContainer.Insert(
        normalMapKey,
        make_unique<RenderTexture>(
            context.device, context.descriptorHeap, context.screenWidth, context.screenHeight,
            1, 1, TextureDimension::Tex2D,
            RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R8G8B8A8_UNORM, TextureState::Common, &normalOptClear)
    );

    size_t motionMapKey = hash<string>()("MotionMap");
    mMotionMap = Globals::RenderTextureContainer.Insert(
        motionMapKey,
        make_unique<RenderTexture>(
            context.device, context.descriptorHeap, context.screenWidth, context.screenHeight,
            1, 1, TextureDimension::Tex2D,
            RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16G16_FLOAT, TextureState::Common)
    );

    // create signature
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::ObjectConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::MaterialConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)RootSignatureParam::CameraConstant].InitAsConstantBufferView(2);
    slotRootParameter[(int)RootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

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

    const D3D_SHADER_MACRO macros[] =
    {
#ifdef REVERSE_Z
            "REVERSE_Z", "1",
#endif
        NULL, NULL
    };
    // create pso
    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "PreZPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "PreZPass.hlsl",
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
    basePsoDesc.NumRenderTargets = 2;
    basePsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    basePsoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16_FLOAT;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = context.depthStencilFormat;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));
}

void PreZPass::PreprocessPass(const GraphicContext& context)
{

}

void PreZPass::DrawPass(const GraphicContext& context)
{
    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetPipelineState(mPSO.Get());

    RenderTexture* depthTarget = GameApp::GetApp()->GetDepthStencilTarget();
    depthTarget->TransitionTo(context.commandList, TextureState::Write);

    mNormalMap->Clear(context.commandList, 0, 0);
    mNormalMap->TransitionTo(context.commandList, TextureState::Write);

    mMotionMap->Clear(context.commandList, 0, 0);
    mMotionMap->TransitionTo(context.commandList, TextureState::Write);

    RenderTexture::SetRenderTargets(context.commandList,
        {
            RenderTarget(mNormalMap),
            RenderTarget(mMotionMap)
        },
        RenderTarget(depthTarget)
    );

    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::CameraConstant,
        context.frameResource->ConstantCamera->GetElementGPUAddress());

    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::Texture2DTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

    Camera* camera = context.scene->GetCamera();
    camera->UpdateViewMatrix();
    const vector<Instance*>& renderInstances = context.scene->GetVisibleRenderInstances(camera->GetFrustum(), camera->GetInvView(), camera->GetPosition(), camera->GetLook());

    int currMaterialID = -1;
    for (size_t i = 0; i < renderInstances.size(); i++)
    {
        Instance* instance = renderInstances[i];
        context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::ObjectConstant,
            context.frameResource->ConstantObject->GetElementGPUAddress(instance->GetID()));

        // set material if need
        if (currMaterialID != instance->GetMaterial()->GetID())
        {
            currMaterialID = instance->GetMaterial()->GetID();
            context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::MaterialConstant, context.frameResource->ConstantMaterial->GetElementGPUAddress(currMaterialID));
        }

        instance->GetMesh()->Draw(context.commandList);
    }
}

void PreZPass::ReleasePass(const GraphicContext& context)
{

}