#include "stdafx.h"
#include "opaqueLitPass.h"
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

void OpaqueLitPass::PreparePass(const GraphicContext& context)
{
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE texcubeTable;
    texcubeTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 1);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::ObjectConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::MaterialConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)RootSignatureParam::CameraConstant].InitAsConstantBufferView(2);
    slotRootParameter[(int)RootSignatureParam::LightConstant].InitAsConstantBufferView(3);
    slotRootParameter[(int)RootSignatureParam::IBLConstant].InitAsConstantBufferView(4);
    slotRootParameter[(int)RootSignatureParam::ShadowConstant].InitAsConstantBufferView(5);
    slotRootParameter[(int)RootSignatureParam::RenderTargetConstant].InitAsConstantBufferView(6);

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

    ComPtr<ID3DBlob> vs = CompileShader(Globals::ShaderPath / "OpaqueLitPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps = CompileShader(Globals::ShaderPath / "OpaqueLitPass.hlsl",
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
    basePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    basePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    basePsoDesc.SampleMask = UINT_MAX;
    basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    basePsoDesc.NumRenderTargets = 1;
    basePsoDesc.RTVFormats[0] = context.backBufferFormat;
    basePsoDesc.SampleDesc.Count = 1;
    basePsoDesc.SampleDesc.Quality = 0;
    basePsoDesc.DSVFormat = context.depthStencilFormat;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&basePsoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));

    mRTConstant = make_unique<UploadBuffer<RenderTargetParamConstant, true>>(context.device);

    mBRDFLUT = Texture::GetOrLoad(Globals::ImagePath / "IBL_BRDF_LUT.png", false, context);

    size_t rtKey = hash<string>()("OpaqueRT");
    mRenderTarget = Globals::RenderTextureContainer.Insert(
        rtKey,
        make_unique<RenderTexture>(
            context.device, context.descriptorHeap,
            context.screenWidth, context.screenHeight,
            1, 1, TextureDimension::Tex2D,
            RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R16G16B16A16_FLOAT)
    );
}

void OpaqueLitPass::DrawPass(const GraphicContext& context)
{
    RenderTexture* depthTarget = GameApp::GetApp()->GetDepthStencilTarget();
    depthTarget->TransitionTo(context.commandList, TextureState::Write);

    mRenderTarget->Clear(context.commandList, 0, 0);
    mRenderTarget->TransitionTo(context.commandList, TextureState::Write);
    mRenderTarget->SetAsRenderTarget(context.commandList, 0, 0, *depthTarget, 0, 0);

    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetPipelineState(mPSO.Get());

    mIBLParam.IBLEnable = context.renderOption->IBLEnable ? 1 : 0;

    LitPassData* passData = dynamic_cast<LitPassData*>(context.frameResource->GetOrCreate(this,
        [&]() {
        return make_unique<LitPassData>(context.device);
    }));
    passData->ssaoConstant->CopyData(mIBLParam);

    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::IBLConstant,
        passData->ssaoConstant->GetElementGPUAddress());

    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::CameraConstant,
        context.frameResource->ConstantCamera->GetElementGPUAddress());
    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::LightConstant,
        context.frameResource->ConstantLight->GetElementGPUAddress());
    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::ShadowConstant,
        context.frameResource->ConstantShadow->GetElementGPUAddress());
    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::RenderTargetConstant,
        mRTConstant->GetElementGPUAddress());
    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::Texture2DTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());
    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::TextureCubeTable,
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

    Camera* camera = context.scene->GetCamera();
    camera->UpdateViewMatrix();
    const vector<Instance*>& renderInstances = context.scene->GetVisibleRenderInstances(camera->GetFrustum(), camera->GetInvView(), camera->GetPosition(), camera->GetLook());

    int currMaterialID = -1;
    for (size_t i = 0; i < renderInstances.size(); i++)
    {
        Instance* instance = renderInstances[i];
        context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::ObjectConstant, context.frameResource->ConstantObject->GetElementGPUAddress(instance->GetID()));

        // set material if need
        if (currMaterialID != instance->GetMaterial()->GetID())
        {
            currMaterialID = instance->GetMaterial()->GetID();
            context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::MaterialConstant, context.frameResource->ConstantMaterial->GetElementGPUAddress(currMaterialID));
        }

        instance->GetMesh()->Draw(context.commandList);
    }

}

void OpaqueLitPass::ReleasePass(const GraphicContext& context)
{

}

void OpaqueLitPass::PreprocessPass(const GraphicContext& context)
{
    size_t irradianceKey = hash<string>()("IrradianceMap");
    ITexture* irradianceMap = Globals::RenderTextureContainer.Get(irradianceKey);

    size_t prefilterKey = hash<string>()("PrefilterMap");
    ITexture* prefilterMap = Globals::RenderTextureContainer.Get(prefilterKey);

    size_t SSAOKey = hash<string>()("SSAO");
    ITexture* ssaoMap = Globals::UATextureContainer.Get(SSAOKey);

    mIBLParam.BRDFLUTIndex = mBRDFLUT->GetSrvDescriptorData().HeapIndex;
    mIBLParam.IrradianceMapIndex = irradianceMap->GetSrvDescriptorData().HeapIndex;
    mIBLParam.PrefilterMapIndex = prefilterMap->GetSrvDescriptorData().HeapIndex;
    mIBLParam.PrefilterMapMipCount = prefilterMap->GetMipCount();
    mIBLParam.SSAOMapIndex = ssaoMap->GetSrvDescriptorData().HeapIndex;
    mIBLParam.IBLEnable = 1;

    context.renderOption->IBLEnable = true;

    RenderTargetParamConstant renderTargetParam;
    renderTargetParam.RenderTargetSize = XMFLOAT4(context.screenWidth, context.screenHeight, 1.0f / context.screenWidth, 1.0f / context.screenHeight);
    mRTConstant->CopyData(renderTargetParam);
}