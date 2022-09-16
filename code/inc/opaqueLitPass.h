#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class Texture;
class RenderTexture;
class RenderTargetParamConstant;

class OpaqueLitPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    CBUFFER IBLConstant
    {
        int IrradianceMapIndex;
        int PrefilterMapIndex;
        int BRDFLUTIndex;
        int PrefilterMapMipCount;

        int SSAOMapIndex;
        int IBLEnable;
    };

    struct LitPassData : public PassData
    {
        std::unique_ptr<UploadBuffer<IBLConstant, true>> ssaoConstant;

        LitPassData(ID3D12Device* device)
        {
            ssaoConstant = std::make_unique<UploadBuffer<IBLConstant, true>>(device);
        }
    };

    enum class RootSignatureParam
    {
        ObjectConstant = 0,
        MaterialConstant = 1,
        CameraConstant = 2,
        LightConstant = 3,
        IBLConstant = 4,
        ShadowConstant = 5,
        RenderTargetConstant = 6,

        Texture2DTable = 7,
        TextureCubeTable = 8,
        

        COUNT = 9
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    IBLConstant mIBLParam;
    std::unique_ptr<UploadBuffer<RenderTargetParamConstant, true>> mRTConstant;

    Texture* mBRDFLUT;

    RenderTexture* mRenderTarget;
};