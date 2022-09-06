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

struct ShadowPassData : public PassData
{
    XMVECTOR lastSunPosition[MAX_CASCADE_COUNT];

    ShadowPassData()
    {
        for (size_t i = 0; i < MAX_CASCADE_COUNT; i++)
        {
            lastSunPosition[i] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
};

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
    const D3D_SHADER_MACRO macros1[] =
    {
#ifdef REVERSE_Z
        "REVERSE_Z", "1",
#endif
        //"SHADOW_PANCAKING", "1",
        NULL, NULL
    };

    ComPtr<ID3DBlob> vs1 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros1, "vs_shadow", "vs_5_1");
    ComPtr<ID3DBlob> ps1 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros1, "ps_shadow", "ps_5_1");

    const D3D_SHADER_MACRO macros2[] =
    {
#ifdef REVERSE_Z
        "REVERSE_Z", "1",
#endif
        NULL, NULL
    };

    ComPtr<ID3DBlob> vs2 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros2, "vs_point_shadow", "vs_5_1");
    ComPtr<ID3DBlob> ps2 = CompileShader(Globals::ShaderPath / "ShadowCasterPass.hlsl",
        macros2, "ps_point_shadow", "ps_5_1");

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
    shadowPsoDesc.DSVFormat = DXGI_FORMAT_D16_UNORM;

    ThrowIfFailed(context.device->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(mShadowPSO.GetAddressOf())));

    // point shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pointShadowPsoDesc = shadowPsoDesc;
    pointShadowPsoDesc.NumRenderTargets = 1;
    pointShadowPsoDesc.RTVFormats[0] = DXGI_FORMAT_R16_UNORM;
    pointShadowPsoDesc.DSVFormat = DXGI_FORMAT_D16_UNORM;
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

// ======================== draw shadows ==================================

void ShadowPass::DrawSunShadow(const GraphicContext& context)
{
    UINT sunConstantOffset = 0;

    const DirectionalLight& sun = context.scene->GetDirectionalLight();
    const Camera* camera = context.scene->GetCamera();
    int cascadeCount = camera->GetCascadeCount();
    array<float, MAX_CASCADE_COUNT> cascadeDistance = camera->GetShadowCascadeDistance();

    // clear shadow map & set part shadow constant
    for (size_t i = 0; i < cascadeCount; i++)
    {
        sun.ShadowMaps[i]->Clear(context.commandList, 0, 0);
        mShadowConstant.ShadowCascade[i].shadowMapIndex = sun.ShadowMaps[i]->GetSrvDescriptorData().HeapIndex;
        mShadowConstant.ShadowCascade[i].cascadeDistance = cascadeDistance[i];
    }
    mShadowConstant.sunCastShadow = sun.castShadow;

    if (!sun.castShadow) {
        return;
    }

    ShadowPassData* passData = dynamic_cast<ShadowPassData*>(context.frameResource->GetOrCreate(this, 
        []() {
            return make_unique<ShadowPassData>();
        }
    ));


    const XMMATRIX sunWorldToLocal = XMMatrixLookToLH(
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 
        XMLoadFloat3(&sun.Direction),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    XMVECTOR determinant;
    const XMMATRIX sunLocalToWorld = XMMatrixInverse(
        &determinant,
        sunWorldToLocal);

    array<XMVECTOR, 4> nearCorners, farCorners;
    nearCorners = camera->GetFrustumPlaneConers(camera->GetNearZ());
    for (size_t i = 0; i < cascadeCount; i++)
    {
        float distance = cascadeDistance[i];
        farCorners = camera->GetFrustumPlaneConers(distance);
        float farPlaneDiagonal = XMVectorGetX(XMVector3Length(farCorners[0] - farCorners[2]));
        float volumeDiagonal = XMVectorGetX(XMVector3Length(nearCorners[0] - farCorners[2]));
        float viewVolumeSize = max(farPlaneDiagonal, volumeDiagonal);
        float pixelSize = viewVolumeSize / sun.ShadowMaps[i]->GetWidth();

        // calculate cascade matrices
        XMVECTOR minPoint, maxPoint;
        minPoint = maxPoint = XMVector4Transform(nearCorners[0], sunWorldToLocal);
        for (size_t j = 0; j < 4; j++)
        {
            XMVECTOR pNear = XMVector4Transform(nearCorners[j], sunWorldToLocal);
            XMVECTOR pFar = XMVector4Transform(farCorners[j], sunWorldToLocal);
            minPoint = XMVectorMin(pNear, minPoint);
            minPoint = XMVectorMin(pFar, minPoint);
            maxPoint = XMVectorMax(pNear, maxPoint);
            maxPoint = XMVectorMax(pFar, maxPoint);
        }

        swap(nearCorners, farCorners);

        XMVECTOR sunLocalPos = 0.5f * (minPoint + maxPoint);
        sunLocalPos = XMVectorSetZ(sunLocalPos, XMVectorGetZ(minPoint) - mSunNearPushBack);
        XMVECTOR movement = sunLocalPos - passData->lastSunPosition[i];
        movement = XMVectorFloor(movement / pixelSize) * pixelSize;
        sunLocalPos = passData->lastSunPosition[i] + movement;
        passData->lastSunPosition[i] = sunLocalPos;

        XMVECTOR sunWorldPos = XMVector3Transform(sunLocalPos, sunLocalToWorld);
        
        XMMATRIX sunView = XMMatrixLookToLH(
            sunWorldPos,
            XMLoadFloat3(&sun.Direction),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

        XMVECTOR determinant;
        XMMATRIX sunInvView = XMMatrixInverse(
            &determinant,
            sunView);

#ifdef REVERSE_Z
        XMMATRIX sunProject = XMMatrixOrthographicLH(viewVolumeSize, viewVolumeSize, viewVolumeSize + mSunNearPushBack, 0.0f);
#else
        XMMATRIX sunProject = XMMatrixOrthographicLH(viewVolumeSize, viewVolumeSize, 0.0f, viewVolumeSize + mSunNearPushBack);
#endif 
        XMMATRIX sunVP = XMMatrixMultiply(sunView, sunProject);
        
        // set culling bounding oriented box
        BoundingOrientedBox obbx;
        obbx.Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        obbx.Extents = XMFLOAT3(viewVolumeSize, viewVolumeSize, viewVolumeSize + mSunNearPushBack);
        obbx.Center = XMFLOAT3(0.0f, 0.0f, 0.5f * obbx.Extents.z);

        // set part cascade shadow constant
        XMStoreFloat4x4(&mShadowConstant.ShadowCascade[i].shadowTransform,
            XMMatrixTranspose(XMMatrixMultiply(sunVP, mTexCoordTransform)));
        mShadowConstant.ShadowCascade[i].shadowBias = sun.ShadowBias * pixelSize * 1.4142136f;
        
        // set shadow caster constant
        ShadowCasterConstant sunShadowConstant;
        XMStoreFloat4x4(&sunShadowConstant.LightViewProject, XMMatrixTranspose(sunVP));
        
#ifdef REVERSE_Z
        sunShadowConstant.NearZ = viewVolumeSize + mSunNearPushBack;
#else
        sunShadowConstant.NearZ = 0.0f;
#endif // REVERSE_Z

        context.frameResource->ConstantShadowCaster->CopyData(sunShadowConstant, sunConstantOffset + i);
        context.commandList->SetGraphicsRootConstantBufferView(
            (int)RootSignatureParam::ShadowCasterConstant,
            context.frameResource->ConstantShadowCaster->GetElementGPUAddress(sunConstantOffset + i));

        // set render target
        sun.ShadowMaps[i]->TransitionTo(context.commandList, TextureState::Write);
        sun.ShadowMaps[i]->SetAsRenderTarget(context.commandList, 0, 0);

        // draw scene to shadow map
        auto instances = context.scene->GetVisibleRenderInstances(
            obbx, sunInvView,
            sunWorldPos, XMLoadFloat3(&sun.Direction));
        DrawInstanceShadow(context, instances);

        sun.ShadowMaps[i]->TransitionTo(context.commandList, TextureState::Read);
    }

}

void ShadowPass::DrawSpotLightShadow(const GraphicContext& context)
{
    UINT spotConstantOffset = MAX_CASCADE_COUNT;

    const auto& spotLights = context.scene->GetVisibleSpotLights();
    for (size_t i = 0; i < spotLights.size(); i++)
    {
        const SpotLight* spotLight = spotLights[i];
        spotLight->ShadowMap->Clear(context.commandList, 0, 0);

        // set shadow constant
        XMFLOAT4X4 viewProject;
        XMStoreFloat4x4(&viewProject, XMMatrixTranspose(XMMatrixMultiply(spotLight->ViewProject, mTexCoordTransform)));
        mShadowConstant.ShadowSpot[i] = {
            viewProject,
            static_cast<int>(spotLight->ShadowMap->GetSrvDescriptorData().HeapIndex),
            CalcPerspectiveShadowBias(spotLight->ShadowBias, spotLight->OutterAngle, spotLight->ShadowMap->GetWidth()),
            spotLight->castShadow,
        };

        if (!spotLight->castShadow)
        {
            continue;
        }
        
        // set shadow caster constant
        ShadowCasterConstant spotShadowConstant;
        XMStoreFloat4x4(&spotShadowConstant.LightViewProject, XMMatrixTranspose(spotLight->ViewProject));
        spotShadowConstant.LightPosition = spotLight->Position;
        spotShadowConstant.LightInvRange = 1.0f / spotLight->Range ;
        context.frameResource->ConstantShadowCaster->CopyData(spotShadowConstant, spotConstantOffset + i);
        context.commandList->SetGraphicsRootConstantBufferView(
            (int)RootSignatureParam::ShadowCasterConstant,
            context.frameResource->ConstantShadowCaster->GetElementGPUAddress(spotConstantOffset + i));
        
        // draw scene to shadow map
        spotLight->ShadowMap->TransitionTo(context.commandList, TextureState::Write);
        spotLight->ShadowMap->SetAsRenderTarget(context.commandList, 0, 0);

        const auto& instances = context.scene->GetVisibleRenderInstances(
            spotLight->Frustum, spotLight->InvView,
            XMLoadFloat3(&spotLight->Position), XMLoadFloat3(&spotLight->Direction)
        );
        
        DrawInstanceShadow(context, instances);

        spotLight->ShadowMap->TransitionTo(context.commandList, TextureState::Read);
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

        // set shadow constant
        mShadowConstant.ShadowPoint[i] = {
            static_cast<int>(pointLight->ShadowMap->GetSrvDescriptorData().HeapIndex),
            CalcPerspectiveShadowBias(pointLight->ShadowBias, M_PI_2, pointLight->ShadowMap->GetWidth()),
            pointLight->castShadow
        };

        if (!pointLight->castShadow) {
            continue;
        }

        pointLight->ShadowMap->TransitionTo(context.commandList, TextureState::Write);

        for (size_t face = 0; face < 6; face++)
        {
            // set shadow caster constant
            ShadowCasterConstant pointShadowConstant;
            XMStoreFloat4x4(&pointShadowConstant.LightViewProject, XMMatrixTranspose(pointLight->GetViewProject(face)));
            pointShadowConstant.LightPosition = pointLight->Position;
            pointShadowConstant.LightInvRange = 1.0f / pointLight->Range;
            context.frameResource->ConstantShadowCaster->CopyData(pointShadowConstant, pointConstantOffset + i * 6 + face);
            context.commandList->SetGraphicsRootConstantBufferView(
                (int)RootSignatureParam::ShadowCasterConstant,
                context.frameResource->ConstantShadowCaster->GetElementGPUAddress(pointConstantOffset + i * 6 + face));
            
            // draw scene to shadow map
            mShadowDepthMap->Clear(context.commandList, 0, 0);
         
            pointLight->ShadowMap->SetAsRenderTarget(context.commandList, face, 0, *mShadowDepthMap, 0, 0);

            const auto& instances = context.scene->GetVisibleRenderInstances(
                pointLight->Frustum, pointLight->GetInvView(face),
                XMLoadFloat3(&pointLight->Position), GetCubeFaceNormal(face)
            );

            DrawInstanceShadow(context, instances);
        }

        pointLight->ShadowMap->TransitionTo(context.commandList, TextureState::Read);
    }
}

void ShadowPass::UpdateShadowConstant(const GraphicContext& context)
{
    mShadowConstant.ShadowMaxDistance = context.scene->GetCamera()->GetShadowMaxDistance();
    mShadowConstant.cascadeCount = context.scene->GetCamera()->GetCascadeCount();
    context.frameResource->ConstantShadow->CopyData(mShadowConstant);
}

float ShadowPass::CalcPerspectiveShadowBias(float baseBias, float fovY, float resolution)
{
    float texelSizeAtDepthOne = 2.0f / (tanf(0.5f * fovY) * resolution);
    return baseBias * texelSizeAtDepthOne * 1.4142136f;
}

void ShadowPass::DrawInstanceShadow(const GraphicContext& context, const vector<Instance*>& instances)
{
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
            context.commandList->SetGraphicsRootConstantBufferView(
                (int)RootSignatureParam::MaterialConstant,
                context.frameResource->ConstantMaterial->GetElementGPUAddress(currMaterialID));
        }

        instance->GetMesh()->Draw(context.commandList);
    }
}