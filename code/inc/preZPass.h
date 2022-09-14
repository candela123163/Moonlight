#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;

class PreZPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:

    enum class RootSignatureParam
    {
        ObjectConstant = 0,
        MaterialConstant = 1,
        CameraConstant = 2,
        Texture2DTable = 3,

        COUNT = 4
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    RenderTexture* mNormalMap;
    RenderTexture* mMotionMap;
};