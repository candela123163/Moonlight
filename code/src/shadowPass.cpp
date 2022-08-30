#include "stdafx.h"
#include "shadowPass.h"
#include "globals.h"
#include "mesh.h"
#include "scene.h"
#include "light.h"
#include "texture.h"
using namespace DirectX;
using namespace std;

const XMMATRIX ShadowPass::mTexCoordTransform = XMMATRIX(
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f
);

void ShadowPass::PreparePass(const GraphicContext& context)
{
    CD3DX12_DESCRIPTOR_RANGE tex2dTable;
    tex2dTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, HEAP_COUNT_SRV, 0, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[(int)RootSignatureParam::COUNT];
    slotRootParameter[(int)RootSignatureParam::ObjectConstant].InitAsConstantBufferView(0);
    slotRootParameter[(int)RootSignatureParam::MaterialConstant].InitAsConstantBufferView(1);
    slotRootParameter[(int)RootSignatureParam::ShadowCasterConstant].InitAsConstantBufferView(2);
    
    slotRootParameter[(int)RootSignatureParam::Texture2DTable].InitAsDescriptorTable(1, &tex2dTable, D3D12_SHADER_VISIBILITY_PIXEL);

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

    ComPtr<ID3DBlob> vs1 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros, "vs_shadow", "vs_5_1");
    ComPtr<ID3DBlob> ps1 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros, "ps_shadow", "ps_5_1");

    ComPtr<ID3DBlob> vs2 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros, "vs_point_shadow", "vs_5_1");
    ComPtr<ID3DBlob> ps2 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros, "ps_point_shadow", "ps_5_1");

    // directional & spot shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc;
    ZeroMemory(&shadowPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    shadowPsoDesc.InputLayout = { Vertex::InputLayout.data(), static_cast<UINT>(Vertex::InputLayout.size()) };
    shadowPsoDesc.pRootSignature = mSignature.Get();
    shadowPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs1->GetBufferPointer()),
        vs1->GetBufferSize()
    };
    shadowPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps1->GetBufferPointer()),
        ps1->GetBufferSize()
    };
    shadowPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    shadowPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    shadowPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

#ifdef REVERSE_Z
    shadowPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
#endif

    shadowPsoDesc.SampleMask = UINT_MAX;
    shadowPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    shadowPsoDesc.NumRenderTargets = 0;
    shadowPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    shadowPsoDesc.SampleDesc.Count = 1;
    shadowPsoDesc.SampleDesc.Quality = 0;
    shadowPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(mShadowPSO.GetAddressOf())));

    // point shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pointShadowPsoDesc = shadowPsoDesc;
    pointShadowPsoDesc.NumRenderTargets = 1;
    pointShadowPsoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
    pointShadowPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pointShadowPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(vs2->GetBufferPointer()),
        vs2->GetBufferSize()
    };
    pointShadowPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(ps2->GetBufferPointer()),
        ps2->GetBufferSize()
    };

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&pointShadowPsoDesc, IID_PPV_ARGS(mPointShadowPSO.GetAddressOf())));

    
    mShadowDepthMap = make_unique<RenderTexture>(context.device, context.descriptorHeap, 
        POINT_LIGHT_SHADOWMAP_RESOLUTION, POINT_LIGHT_SHADOWMAP_RESOLUTION,
        6, 1, TextureDimension::Tex2D, RenderTextureUsage::DepthBuffer, DXGI_FORMAT_D32_FLOAT);
}

void ShadowPass::PreprocessPass(const GraphicContext& context)
{

}

void ShadowPass::DrawPass(const GraphicContext& context)
{
    context.commandList->SetGraphicsRootSignature(mSignature.Get());
    context.commandList->SetGraphicsRootDescriptorTable((int)RootSignatureParam::Texture2DTable, 
        context.descriptorHeap->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

    context.commandList->SetPipelineState(mShadowPSO.Get());
    DrawSunShadow(context);
    DrawSpotLightShadow(context);

    context.commandList->SetPipelineState(mPointShadowPSO.Get());
    DrawPointLightShadow(context);

    UpdateShadowConstant(context);
}

void ShadowPass::ReleasePass(const GraphicContext& context)
{

}

void ShadowPass::DrawSunShadow(const GraphicContext& context)
{

}

void ShadowPass::DrawSpotLightShadow(const GraphicContext& context)
{
    UINT spotConstantOffset = MAX_CASCADE_COUNT;

    const auto& spotLights = context.scene->GetVisibleSpotLights();
    for (size_t i = 0; i < spotLights.size(); i++)
    {
        const SpotLight* spotLight = spotLights[i];
        spotLight->ShadowMap->Clear(context.commandList, 0, 0);

        XMFLOAT4X4 viewProject;
        XMStoreFloat4x4(&viewProject, XMMatrixTranspose(XMMatrixMultiply(spotLight->ViewProject, mTexCoordTransform)));
        mShadowConstant.ShadowSpot[i] = {
            viewProject,
            static_cast<int>(spotLight->ShadowMap->GetSrvDescriptorData().HeapIndex),
            CalcPerspectiveNormalBias(spotLight->ShadowBias, spotLight->OutterAngle, spotLight->ShadowMap->GetWidth()),
            spotLight->castShadow,
            0
        };

        if (!spotLight->castShadow)
        {
            continue;
        }
        
        ShadowCasterConstant spotShadowConstant;
        XMStoreFloat4x4(&spotShadowConstant.LightViewProject, XMMatrixTranspose(spotLight->ViewProject));
        
        spotShadowConstant.LightPosition = spotLight->Position;
        spotShadowConstant.LightInvRange = 1.0f / spotLight->Range ;
        context.frameResource->ConstantShadowCaster->CopyData(spotShadowConstant, spotConstantOffset + i);
        context.commandList->SetGraphicsRootConstantBufferView(
            (int)RootSignatureParam::ShadowCasterConstant,
            context.frameResource->ConstantShadowCaster->GetElementGPUAddress(spotConstantOffset + i));
        
        spotLight->ShadowMap->TransitionTo(context.commandList, RenderTextureState::Write);
        spotLight->ShadowMap->SetAsRenderTarget(context.commandList, 0, 0);

        const auto& instances = context.scene->GetVisibleRenderInstances(
            spotLight->Frustum, spotLight->InvView,
            XMLoadFloat3(&spotLight->Position), XMLoadFloat3(&spotLight->Direction)
        );
        
        int currMaterialID = -1;
        for (auto instance : instances)
        {
            context.commandList->SetGraphicsRootConstantBufferView(
                (int)RootSignatureParam::ObjectConstant,
                context.frameResource->ConstantObject->GetElementGPUAddress(instance->GetID()));
            // set material if need
            if (currMaterialID != instance->GetMaterial()->GetID())
            {
                currMaterialID = instance->GetMaterial()->GetID();
                context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::MaterialConstant,
                    context.frameResource->ConstantMaterial->GetElementGPUAddress(currMaterialID));
            }

            instance->GetMesh()->Draw(context.commandList);
        }

        spotLight->ShadowMap->TransitionTo(context.commandList, RenderTextureState::Read);
    }
}

void ShadowPass::DrawPointLightShadow(const GraphicContext& context)
{
    UINT pointConstantOffset = MAX_CASCADE_COUNT + MAX_SPOT_LIGHT_COUNT;

    const auto& pointLights = context.scene->GetVisiblePointLights();
    for (size_t i = 0; i < pointLights.size(); i++)
    {
        const PointLight* pointLight = pointLights[i];
        for (size_t face = 0; face < 6; face++)
        {
            pointLight->ShadowMap->Clear(context.commandList, face, 0);
        }

        mShadowConstant.ShadowPoint[i] = {
            static_cast<int>(pointLight->ShadowMap->GetSrvDescriptorData().HeapIndex),
            CalcPerspectiveNormalBias(pointLight->ShadowBias, M_PI_2, pointLight->ShadowMap->GetWidth()),
            pointLight->castShadow,
            0
        };

        if (!pointLight->castShadow) {
            continue;
        }

        pointLight->ShadowMap->TransitionTo(context.commandList, RenderTextureState::Write);

        for (size_t face = 0; face < 6; face++)
        {
            ShadowCasterConstant pointShadowConstant;
            XMStoreFloat4x4(&pointShadowConstant.LightViewProject, XMMatrixTranspose(pointLight->GetViewProject(face)));

            pointShadowConstant.LightPosition = pointLight->Position;
            pointShadowConstant.LightInvRange = 1.0f / pointLight->Range;
            context.frameResource->ConstantShadowCaster->CopyData(pointShadowConstant, pointConstantOffset + i * 6 + face);
            context.commandList->SetGraphicsRootConstantBufferView(
                (int)RootSignatureParam::ShadowCasterConstant,
                context.frameResource->ConstantShadowCaster->GetElementGPUAddress(pointConstantOffset + i * 6 + face));

            mShadowDepthMap->Clear(context.commandList, 0, 0);
         
            pointLight->ShadowMap->SetAsRenderTarget(context.commandList, face, 0, *mShadowDepthMap, 0, 0);

            const auto& instances = context.scene->GetVisibleRenderInstances(
                pointLight->Frustum, pointLight->GetInvView(face),
                XMLoadFloat3(&pointLight->Position), GetCubeFaceNormal(face)
            );

            int currMaterialID = -1;
            for (auto instance : instances)
            {
                context.commandList->SetGraphicsRootConstantBufferView(
                    (int)RootSignatureParam::ObjectConstant,
                    context.frameResource->ConstantObject->GetElementGPUAddress(instance->GetID()));
                // set material if need
                if (currMaterialID != instance->GetMaterial()->GetID())
                {
                    currMaterialID = instance->GetMaterial()->GetID();
                    context.commandList->SetGraphicsRootConstantBufferView((int)RootSignatureParam::MaterialConstant,
                        context.frameResource->ConstantMaterial->GetElementGPUAddress(currMaterialID));
                }

                instance->GetMesh()->Draw(context.commandList);
            }
        }

        pointLight->ShadowMap->TransitionTo(context.commandList, RenderTextureState::Read);
    }
}

void ShadowPass::UpdateShadowConstant(const GraphicContext& context)
{
    mShadowConstant.ShadowMaxDistance = context.scene->GetCamera()->GetShadowMaxDistance();
    context.frameResource->ConstantShadow->CopyData(mShadowConstant);
}

float ShadowPass::CalcPerspectiveNormalBias(float baseBias, float fovY, float resolution)
{
    float texelSizeAtDepthOne = 2.0f / (tanf(0.5f * fovY) * resolution);
    return baseBias * texelSizeAtDepthOne * 1.4142136f;
}