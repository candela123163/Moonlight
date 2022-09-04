#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;
class UnorderAccessTexture;
class Instance;

class HBAOPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    void CreateResource(const GraphicContext& context);

    void CreateNormalSignaturePSO(const GraphicContext& context);
    void CreateHBAOSignaturePSO(const GraphicContext& context);
    void CreateBlurSignaturePSO(const GraphicContext& context);

    void DrawNormalDepth(const GraphicContext& context);
    void DrawHBAO(const GraphicContext& context);
    void BlurHBAO(const GraphicContext& context);

    enum class NormalRootSignatureParam
    {
        ObjectConstant = 0,
        MaterialConstant = 1,
        CameraConstant = 2,
        Texture2DTable = 3,

        COUNT = 4
    };

private:
    ComPtr<ID3D12RootSignature> mNormalDepthSignature = nullptr;
    ComPtr<ID3D12RootSignature> mHBAOSignature = nullptr;
    ComPtr<ID3D12RootSignature> mBlurSignature = nullptr;

    ComPtr<ID3D12PipelineState> mNormalDepthPSO = nullptr;
    ComPtr<ID3D12PipelineState> mHBAOPSO = nullptr;
    ComPtr<ID3D12PipelineState> mBlurPSO = nullptr;

    // this pass own these resources
    // since there is no need to be accessed in other pass
    std::unique_ptr<RenderTexture> mNormalMap = nullptr;
    std::unique_ptr<RenderTexture> mHBAOMap = nullptr;
    std::unique_ptr<UnorderAccessTexture> mHBAOBlurXMap = nullptr;

    // resource container own this resource
    UnorderAccessTexture* mHBAOBlurYMap = nullptr;
};