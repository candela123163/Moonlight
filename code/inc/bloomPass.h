#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;
class UnorderAccessTexture;

class BloomPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    void CreateResource(const GraphicContext& context);
    void CreateSignaturePSO(const GraphicContext& context);

    void DoPrefilter(const GraphicContext& context);
    void DoDownSample(const GraphicContext& context);
    void DoUpSample(const GraphicContext& context);
    void DoCombine(const GraphicContext& context);

    void CalculateBloomParam(float inputWidth, float inputHeight);

    enum class RootSignatureParam
    {
        BloomConstant = 0,
        MipLevelConstant32 = 1,

        BloomChainTexture = 2,
        InputTexture = 3,
        OutputTexture = 4,

        COUNT = 5
    };

    CBUFFER BloomConstant
    {
        DirectX::XMFLOAT4 Threshold;
        
        float Intensity;
    };


private:
    ComPtr<ID3D12RootSignature> mSignature;
    ComPtr<ID3D12PipelineState> mPrefilterPSO;
    ComPtr<ID3D12PipelineState> mDownSamplePSO;
    ComPtr<ID3D12PipelineState> mUpSamplePSO;
    ComPtr<ID3D12PipelineState> mCombinePSO;

    std::unique_ptr<UnorderAccessTexture> mDownSampleChain;
    UnorderAccessTexture* mUpSampleChain;

    RenderTexture* mInputRT;

    BloomConstant mBloomParam;
    std::unique_ptr<UploadBuffer<BloomConstant, true>> mBloomConstant;

    const float mThreshold = 1.0f;
    const float mKnee = 0.5f;
    const float mIntensity = 0.5f;

    const int mMaxChainLength = 12;
    const int mMinChainLength = 2;
    const int mMinDownScaleSize = 2;
    int mIterationCount = 2;

    static constexpr int mNumConst32 = 2;
};