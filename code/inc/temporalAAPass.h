#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;

class TAAPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:

    enum class RootSignatureParam
    {
        CameraConstant = 0,
        TAAConstant = 1,
        Texture2D = 2,

        COUNT = 3
    };

    CBUFFER TAAConstant
    {
        int FrameColorMapIndex;
        int FrameDepthMapIndex;
        int HistoryColorMapIndex;
        int MotionMapIndex;

        DirectX::XMFLOAT2 TexelSize;
        DirectX::XMFLOAT2 HistoryClipBound;

        float MotionWeightFactor;
        float Sharpness;
    };

    struct TAAPassData : public PassData
    {
        std::unique_ptr<UploadBuffer<TAAConstant, true>> ssaoConstant;

        TAAPassData(ID3D12Device* device)
        {
            ssaoConstant = std::make_unique<UploadBuffer<TAAConstant, true>>(device);
        }
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    std::unique_ptr<RenderTexture> mHistory;
    RenderTexture* mTAAOut;

    TAAConstant mTAAParam;
};