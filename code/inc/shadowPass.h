#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "descriptor.h"
using Microsoft::WRL::ComPtr;


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

    enum class RootSignatureParam
    {
        ObjectConstant = 0,
        ShadowCasterConstant = 1,
        Texture2DTable = 2,
       
        COUNT = 3
    };
  

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr< ID3D12PipelineState> mShadowPSO = nullptr;
    ComPtr< ID3D12PipelineState> mPointShadowPSO = nullptr;
};