#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "descriptor.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;

class IBLPreprocessPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;
private:

    enum class RootSignatureParam
    {
        IBLConstant = 0,
        Constant32 = 1,
        Texture2DTable = 2,
        
        COUNT = 3
    };

    struct IBLPreprocessConstant
    {
        DirectX::XMFLOAT4X4 ViewProj;
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    
    ComPtr<ID3D12PipelineState> mIrradiancePSO = nullptr;
    ComPtr<ID3D12PipelineState> mPrefilterPSO = nullptr;

    RenderTexture* mIrradianceMap;
    RenderTexture* mPrefilterMap;

    std::unique_ptr<UploadBuffer<IBLPreprocessConstant, true>> mConstant;

    const UINT mIrradianceMapSize = 64;
    const UINT mPrefilterMapSize = 256;
    const UINT mPrefilterLevel = 5;
};