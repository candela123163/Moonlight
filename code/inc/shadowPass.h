#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "descriptor.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;
class Instance;

class ShadowPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    void DrawSunShadow(const GraphicContext& context);
    void DrawSpotLightShadow(const GraphicContext& context);
    void DrawPointLightShadow(const GraphicContext& context);

    void UpdateShadowConstant(const GraphicContext& context);

    float CalcPerspectiveShadowBias(float baseBias,float fovY, float resolution);

    void DrawInstanceShadow(const GraphicContext& context, const std::vector<Instance*>& instances);

    enum class RootSignatureParam
    {
        ObjectConstant = 0,
        MaterialConstant = 1,
        ShadowCasterConstant = 2,
        Texture2DTable = 3,
       
        COUNT = 4
    };
  

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr< ID3D12PipelineState> mShadowPSO = nullptr;
    ComPtr< ID3D12PipelineState> mPointShadowPSO = nullptr;

    std::unique_ptr<RenderTexture> mShadowDepthMap;

    ShadowConst mShadowConstant;
    
    static const DirectX::XMMATRIX mTexCoordTransform;
    const float mSunNearPushBack = 100.0f;
};