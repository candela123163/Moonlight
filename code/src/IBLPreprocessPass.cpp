#include "stdafx.h"
#include "IBLPreprocessPass.h"
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

void IBLPreprocessPass::PreparePass(const GraphicContext& context)
{
    // ---------- create render texture ---------- 
    size_t IrradianceKey = hash<string>()("IrradianceMap");
    mIrradianceMap = Globals::RenderTextureContainer.Insert(
        IrradianceKey,
        make_unique<RenderTexture>(
            context.device, context.descriptorHeap, mIrradianceMapSize, mIrradianceMapSize,
            6, 1, TextureDimension::CubeMap,
            RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R8G8B8A8_UNORM
            )
    );

    size_t PrefilterKey = hash<string>()("PrefilterMap");
    mPrefilterMap = Globals::RenderTextureContainer.Insert(
        PrefilterKey,
        make_unique<RenderTexture>(
            context.device, context.descriptorHeap, mPrefilterMapSize, mPrefilterMapSize,
            6, mPrefilterLevel, TextureDimension::CubeMap,
            RenderTextureUsage::ColorBuffer, DXGI_FORMAT_R8G8B8A8_UNORM
            )
    );

    // ---------- create constant buffer ---------- 
    mConstant = make_unique<UploadBuffer<IBLPreprocessConstant, true>>(context.device, 6);

    // ---------------  signature ----------------- 
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::IBLConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::Constant32].InitAsConstants(2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[(int)RootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    

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


    // ------------------- pso ------------------- 

    const D3D_SHADER_MACRO macros[] =
    {
#ifdef REVERSE_Z
        "REVERSE_Z", "1",
#endif
        NULL, NULL
    };

    ComPtr<ID3DBlob> vs1 = CompileShader(Globals::ShaderPath / "IrradianceMapPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps1 = CompileShader(Globals::ShaderPath / "IrradianceMapPass.hlsl",
        macros, "ps", "ps_5_1");

    ComPtr<ID3DBlob> vs2 = CompileShader(Globals::ShaderPath / "PrefilterMapPass.hlsl",
        macros, "vs", "vs_5_1");
    ComPtr<ID3DBlob> ps2 = CompileShader(Globals::ShaderPath / "PrefilterMapPass.hlsl",
        macros, "ps", "ps_5_1");


    D3D12_GRAPHICS_PIPELINE_STATE_DESC irradiancePsoDesc;
    ZeroMemory(&irradiancePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    irradiancePsoDesc.InputLayout = { Vertex::InputLayout.data(), static_cast<UINT>(Vertex::InputLayout.size()) };
    irradiancePsoDesc.pRootSignature = mSignature.Get();
    irradiancePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs1->GetBufferPointer()),
        vs1->GetBufferSize()
    };
    irradiancePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps1->GetBufferPointer()),
        ps1->GetBufferSize()
    };
    irradiancePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    // camera is in the skybox mesh, so culling front instead of culling back
    irradiancePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    irradiancePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    irradiancePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    irradiancePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    irradiancePsoDesc.SampleMask = UINT_MAX;
    irradiancePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    irradiancePsoDesc.NumRenderTargets = 1;
    irradiancePsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    irradiancePsoDesc.SampleDesc.Count = 1;
    irradiancePsoDesc.SampleDesc.Quality = 0;
    irradiancePsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&irradiancePsoDesc, IID_PPV_ARGS(mIrradiancePSO.GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC prefilterPsoDesc = irradiancePsoDesc;
    prefilterPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs2->GetBufferPointer()),
        vs2->GetBufferSize()
    };
    prefilterPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps2->GetBufferPointer()),
        ps2->GetBufferSize()
    };
    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&irradiancePsoDesc, IID_PPV_ARGS(mPrefilterPSO.GetAddressOf())));
}

void IBLPreprocessPass::DrawPass(const GraphicContext& context)
{
    //PreprocessPass(context);
}

void IBLPreprocessPass::ReleasePass(const GraphicContext& context)
{

}

void IBLPreprocessPass::PreprocessPass(const GraphicContext& context)
{
    FXMMATRIX project = XMMatrixPerspectiveFovLH(M_PI_2, 1.0, 0.1, 300.0);
    array<FXMMATRIX, 6> views = GenerateCubeViewMatrices(XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0f, 0.0f, 0.0f))));
    const SkyBox& skybox = context.scene->GetSkybox();

    // set matrix constant
    for (size_t i = 0; i < 6; i++)
    {
        IBLPreprocessConstant iblConstant;
        XMStoreFloat4x4(&iblConstant.ViewProj, XMMatrixTranspose(
            XMMatrixMultiply(views[i], project)
        ));
        mConstant->CopyData(iblConstant, i);
    }

    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::Texture2DTable, skybox.texture->GetSrvDescriptorData().GPUHandle);

    // generate irradiance map
    context.commandList->SetPipelineState(mIrradiancePSO.Get());
    mIrradianceMap->TransitionTo(context.commandList, TextureState::Write);
    for (size_t i = 0; i < 6; i++)
    {
        mIrradianceMap->Clear(context.commandList, i, 0);
        mIrradianceMap->SetAsRenderTarget(context.commandList, i, 0);
        context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::IBLConstant, mConstant->GetElementGPUAddress(i));
        skybox.mesh->Draw(context.commandList);
    }
    mIrradianceMap->TransitionTo(context.commandList, TextureState::Read);

    // generate prefilter map
    context.commandList->SetPipelineState(mPrefilterPSO.Get());
    mPrefilterMap->TransitionTo(context.commandList, TextureState::Write);
    float constant32[2];

    for (size_t filterLevel = 0; filterLevel < mPrefilterLevel; filterLevel++)
    {
        // mip 0 for smoothest, mip upper bound for roughest
        float roughness = max(0.001f, 1.0f - static_cast<float>(filterLevel) / (mPrefilterLevel - 1));
        constant32[0] = roughness;
        constant32[1] = mPrefilterMapSize >> filterLevel;
        context.commandList->SetGraphicsRoot32BitConstants((int)RootSignatureParam::Constant32, 2, &constant32, 0);
        for (size_t face = 0; face < 6; face++)
        {
            mPrefilterMap->Clear(context.commandList, face, filterLevel);
            mPrefilterMap->SetAsRenderTarget(context.commandList, face, filterLevel);
            context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::IBLConstant, mConstant->GetElementGPUAddress(face));
            skybox.mesh->Draw(context.commandList);
        }
    }
    mPrefilterMap->TransitionTo(context.commandList, TextureState::Read);
}