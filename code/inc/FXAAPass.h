#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class ITexture;
class UnorderAccessTexture;

class FXAAPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    void CreateResource(const GraphicContext& context);
    void CreateSignaturePSO(const GraphicContext& context);

    enum class RootSignatureParam
    {
        FXAAConstant = 0,

        InputTexture = 1,
        OutputTexture = 2,

        COUNT = 3
    };

    CBUFFER FXAAConstant
    {
        DirectX::XMFLOAT2 TexelSize;
        float FixedThreshold;
        float RelativeThreshold;
        float SubPixelBlending;
    };

private:
    ComPtr<ID3D12RootSignature> mSignature;
    ComPtr<ID3D12PipelineState> mPSO;

    ITexture* mInputRT;
    UnorderAccessTexture* mFXAAOutMap;

    FXAAConstant mFXAAParam;
    std::unique_ptr<UploadBuffer<FXAAConstant, true>> mFXAAConstant;
};