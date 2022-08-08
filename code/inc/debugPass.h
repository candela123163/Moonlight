#pragma once
#include "stdafx.h"
#include "passBase.h"
using Microsoft::WRL::ComPtr;

class DebugPass : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;
};