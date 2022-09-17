#pragma once
#include "stdafx.h"
#include "passBase.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;

class SkyboxPass final : public PassBase
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
        TextureCubeTable = 1,

        COUNT = 2
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    RenderTexture* mRenderTarget = nullptr;
};