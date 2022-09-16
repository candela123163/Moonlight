#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class RenderTexture;
class UnorderAccessTexture;
class Instance;

class SSAOPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    void CreateResource(const GraphicContext& context);

    void CreateSSAOSignaturePSO(const GraphicContext& context);
    void CreateBlurSignaturePSO(const GraphicContext& context);

    void DrawSSAO(const GraphicContext& context);
    void BlurSSAO(const GraphicContext& context);

    enum class SSAORootSignatureParam
    {
        RenderTargetParamConstant = 0,
        CameraConstant = 1,
        SSAOConstant = 2,
        Texture2DTable = 3,

        COUNT = 4
    };

    CBUFFER SSAOConstant
    {
        float   AORadius;
        int     AOSampleCount;
        float   AOPower;
        float   AOFadeRange;

        int     NormalMapIndex;
        int     DepthMapIndex;
    };

    enum class BlurRootSignatureParam
    {
        BlurConstant = 0,
        Texture2DTable = 1,
        AOSrvTable = 2,
        AOUavTable = 3,

        COUNT = 4
    };

    CBUFFER BlurConstant
    {
        int     BlurRadius;
        float   RangeSigma;
        float   DepthSigma;
        float   NormalSigma;

        int     NormalMapIndex;
        int     DepthMapIndex;
        int     MapWidth;
        int     MapHeight;

        float NormalDepthSampleScale;
    };

    struct SSAOPassData : public PassData
    {
        std::unique_ptr<UploadBuffer<SSAOConstant, true>> ssaoConstant;
        
        SSAOPassData(ID3D12Device* device)
        {
            ssaoConstant = std::make_unique<UploadBuffer<SSAOConstant, true>>(device);
        }
    };

private:
    ComPtr<ID3D12RootSignature> mSSAOSignature = nullptr;
    ComPtr<ID3D12RootSignature> mBlurSignature = nullptr;

    ComPtr<ID3D12PipelineState> mSSAOPSO = nullptr;
    ComPtr<ID3D12PipelineState> mBlurXPSO = nullptr;
    ComPtr<ID3D12PipelineState> mBlurYPSO = nullptr;

    // this pass own these resources
    // since there is no need to be accessed in other pass
    std::unique_ptr<RenderTexture> mSSAOMap = nullptr;
    std::unique_ptr<UnorderAccessTexture> mSSAOBlurXMap = nullptr;
    
    // resource container own this resource
    RenderTexture* mNormalMap = nullptr;
    UnorderAccessTexture* mSSAOBlurYMap = nullptr;

    UINT mSSAOWidth, mSSAOHeight;
    SSAOConstant mSSAOParam;

    std::unique_ptr<UploadBuffer<RenderTargetParamConstant, true>> mSSAORTConstant;
    std::unique_ptr<UploadBuffer<BlurConstant, true>> mBlurConstant;

    UINT mBlurCount = 2;
};