#pragma once
#include "stdafx.h"
#include "passBase.h"
using Microsoft::WRL::ComPtr;

class DebugPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    enum class RootSignatureParam
    {
        Texture2DTable = 0,
        TextureCubeTable = 1,
        Constant32Float = 2,
        Constant32UInt = 3,

        COUNT = 4
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;
};