#pragma once
#include "stdafx.h"
#include "util.h"
#include "globals.h"

struct ObjectConstant
{
    DirectX::XMFLOAT4X4 World;
    DirectX::XMFLOAT4X4 InverseTransposedWorld;
};


struct MaterialConstant
{
    DirectX::XMFLOAT4 AlbedoFactor;
    
    float NormalScale;
    int NormalMapped;
    float MetallicFactor;
    float RoughnessFactor;

    int AlbedoMapIndex;
    int NormalMapIndex;
    int MetalRoughMapIndex;
    int pad;
};


struct CameraConstant
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 InvView;
    DirectX::XMFLOAT4X4 Proj;
    DirectX::XMFLOAT4X4 InvProj;
    DirectX::XMFLOAT4X4 ViewProj;
    DirectX::XMFLOAT4X4 InvViewProj;
    DirectX::XMFLOAT4 EyePosW;
    // camera (Znear, Zfar, null, null)
    DirectX::XMFLOAT4 NearFar;
};


struct LightConstant
{
    // Point
    struct PointLight
    {
        DirectX::XMFLOAT3 Color;
        float Intensity;
        
        DirectX::XMFLOAT3 Position;
        float InvRange;
    };

    struct SpotLight
    {
        DirectX::XMFLOAT3 Color;
        float InvRange;

        DirectX::XMFLOAT3 Position;
        float AttenuationFactorA;

        DirectX::XMFLOAT3 Direction;
        float AttenuationFactorB;

        float Intensity;
        DirectX::XMFLOAT3 pad;
    };

    DirectX::XMFLOAT3 SunColor;
    int PointLightCount;

    DirectX::XMFLOAT3 SunDirection;
    int SpotLightCount;

    float SunIntensity;
    DirectX::XMFLOAT3 pad;

    PointLight PointLights[MAX_POINT_LIGHT_COUNT];
    SpotLight SpotLights[MAX_SPOT_LIGHT_COUNT];
};


struct ShadowCasterConstant
{
    DirectX::XMFLOAT4X4 LightViewProject;
    // for point & spot light w is 1 / range
    DirectX::XMFLOAT3 LightPosition;
    float LightInvRange;

    float NearZ;
    DirectX::XMFLOAT3 pad;
};

struct ShadowConst
{
    struct CascadeShadow
    {
        DirectX::XMFLOAT4X4 shadowTransform;
        
        int shadowMapIndex;
        float cascadeDistance;
        float shadowBias;
        float pad;
    };

    struct PointShadow
    {
        int shadowMapIndex;
        float shadowBias;
        int castShadow;
        float pad;
    };

    struct SpotShadow
    {
        DirectX::XMFLOAT4X4 shadowTransform;

        int shadowMapIndex;
        float shadowBias;
        int castShadow;
        float pad;
    };

    CascadeShadow ShadowCascade[MAX_CASCADE_COUNT];
    PointShadow ShadowPoint[MAX_POINT_LIGHT_COUNT];
    SpotShadow ShadowSpot[MAX_SPOT_LIGHT_COUNT];

    float ShadowMaxDistance;
    int cascadeCount;
    int sunCastShadow;
    float pad;
};

struct RenderTargetParamConstant
{
    DirectX::XMFLOAT4 RenderTargetSize;
};

struct PassData
{
    virtual ~PassData() {}
};

class PassBase;

struct FrameResources
{
    FrameResources(ID3D12Device* device);
    FrameResources(const FrameResources& rhs) = delete;
    FrameResources& operator=(const FrameResources& rhs) = delete;
    ~FrameResources() = default;


    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    UINT64 Fence = 0;

    std::unique_ptr<UploadBuffer<RenderTargetParamConstant, true>> ConstantRenderTargetParam;
    std::unique_ptr<UploadBuffer<CameraConstant, true>> ConstantCamera;
    std::unique_ptr<UploadBuffer<LightConstant, true>> ConstantLight;
    std::unique_ptr<UploadBuffer<ShadowConst, true>> ConstantShadow;
    std::unique_ptr<UploadBuffer<ShadowCasterConstant, true>> ConstantShadowCaster;

    std::unique_ptr<UploadBuffer<ObjectConstant, true>> ConstantObject;
    std::unique_ptr<UploadBuffer<MaterialConstant, true>> ConstantMaterial;

    PassData* GetOrCreate(PassBase* key, std::function<std::unique_ptr<PassData>()> createFunc);

private:
    std::unordered_map<PassBase*, std::unique_ptr<PassData>> mPassResource;
};
