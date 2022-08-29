#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
using Microsoft::WRL::ComPtr;

class Texture;

class OpaqueLitPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;

private:
    struct IBLConstant 
    {
        int IrradianceMapIndex;
        int PrefilterMapIndex;
        int BRDFLUTIndex;
        int PrefilterMapMipCount;
    };

    enum class RootSignatureParam
    {
        ObjectConstant = 0,
        MaterialConstant = 1,
        CameraConstant = 2,
        LightConstant = 3,
        IBLConstant = 4,
        ShadowConstant = 5,

        Texture2DTable = 6,
        TextureCubeTable = 7,
        

        COUNT = 8
    };

private:
    ComPtr<ID3D12RootSignature> mSignature = nullptr;
    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    std::unique_ptr<UploadBuffer<IBLConstant, true>> mIBLConstant;
    Texture* mBRDFLUT;
};