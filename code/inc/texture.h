#pragma once
#include "stdafx.h"
#include "descriptor.h"


// ----------- ITexture -------------
enum class TextureDimension 
{
    Tex2D = 0,
    CubeMap = 1,
    Tex2DArray = 2
};

enum class TextureState
{
    Write = 0,
    Read = 1,
    Common = 2,
    CopySrouce = 3,
    CopyDest = 4
};

class ITexture
{
public:
    ITexture() {}
    ITexture(const ITexture&) = delete;
    ITexture(ITexture&&) = delete;
    ITexture& operator=(const ITexture&) = delete;
    ITexture& operator=(ITexture&&) = delete;

    ID3D12Resource* GetResource() const { return mTexture.Get(); }
    void BindSRV(ID3D12Device* device, DescriptorHeap* descriptorHeap);
    void BindSRV(ID3D12Device* device, const DescriptorData& descriptorData);
    bool TransitionTo(ID3D12GraphicsCommandList* commandList, TextureState targetState);
    bool CopyResource(ID3D12GraphicsCommandList* commandList, ITexture& copySource);
    DescriptorData GetSrvDescriptorData() const { return mSrvDescriptorData; }

    UINT GetWidth(int mipLevel = 0) const { return std::max((UINT)1, mWidth >> mipLevel); }
    UINT GetHeight(int mipLevel = 0) const { return std::max((UINT)1, mHeight >> mipLevel); }
    DXGI_FORMAT GetFormat() const { return mFormat; }
    UINT GetDepthCount() const { return mDepthCount; }
    UINT GetMipCount() const { return mMipCount; }
    TextureDimension GetDimension() const { return mDimension; }

    TextureState GetSubResourceState(UINT depthSlice, UINT mipSlice) const { return mSubResourceState[depthSlice * mMipCount + mipSlice]; }
    bool TransitionSubResourceTo(ID3D12GraphicsCommandList* commandList, UINT depthSlice, UINT mipSlice, TextureState targetState);
        
protected:
    virtual void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) = 0;
    virtual bool GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState) = 0;

protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> mTexture = nullptr;
    UINT mWidth = 0, mHeight = 0, mDepthCount = 1, mMipCount = 1;
    TextureDimension mDimension = TextureDimension::Tex2D;
    DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
    DescriptorData mSrvDescriptorData;

    std::vector<TextureState> mSubResourceState;
};


// ----------- Texture -------------
class Texture final : public ITexture
{
public:
    Texture(ID3D12Device* device, 
        DescriptorHeap* descriptorHeap,
        ID3D12CommandQueue* commandQueue,
        const std::filesystem::path& filePath,
        bool sRGB);

    static Texture* GetOrLoad(const std::filesystem::path& texturePath, bool sRGB, const GraphicContext& context);

protected:
    void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) override;
    bool GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState) override;
};


// ----------- RenderTexture -------------
enum class RenderTextureUsage
{
    ColorBuffer = 0,
    DepthBuffer = 1
};

class RenderTexture;
struct RenderTarget
{
    RenderTarget(RenderTexture* rtx, UINT depthSlice_ = 0, UINT mipLevel_ = 0):
        renderTexture(rtx), depthSlice(depthSlice_), mipLevel(mipLevel_) { }

    RenderTexture* renderTexture = nullptr;
    UINT depthSlice = 0;
    UINT mipLevel = 0;
};

class RenderTexture final : public ITexture
{
public:
    RenderTexture(ID3D12Device* device, DescriptorHeap* descriptorHeap,
        UINT width, UINT height,
        UINT depthCount, UINT mipCount, TextureDimension dimension,
        RenderTextureUsage usage, DXGI_FORMAT format,
        TextureState initState = TextureState::Common,
        const D3D12_CLEAR_VALUE* clearValue = nullptr);

    void BindRTV(ID3D12Device* device, 
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
        UINT depthSlice = 0, UINT mipLevel = 0);

    void BindRTV(ID3D12Device* device, DescriptorHeap* descriptorHeap);

    void SetViewPort(ID3D12GraphicsCommandList* commandList, UINT mipLevel = 0);

    DescriptorData GetRtvDescriptorData() const { return mRtvDescriptorData; }

    void SetAsRenderTarget(ID3D12GraphicsCommandList* commandList,
        UINT depthSlice, UINT mipLevel,
        UINT otherRTVNum = 0, const D3D12_CPU_DESCRIPTOR_HANDLE* otherRTVHandles = nullptr,
        const D3D12_CPU_DESCRIPTOR_HANDLE* otherDSVHandles = nullptr
    );

    void SetAsRenderTarget(ID3D12GraphicsCommandList* commandList,
        UINT depthSlice, UINT mipLevel,
        const RenderTexture& other, UINT otherDepthSlice, UINT otherMipLevel
    );

    static void SetRenderTargets(ID3D12GraphicsCommandList* commandList,
        const std::initializer_list<RenderTarget>& renderTargets, RenderTarget depthStencilTarget);

    void Clear(ID3D12GraphicsCommandList* commandList, UINT depthSlice, UINT mipLevel);

    RenderTextureUsage GetUsage() const { return mUsage; }

private:
    void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) override;
    bool GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState) override;

private:
    RenderTextureUsage mUsage;
    DescriptorData mRtvDescriptorData;
    D3D12_CLEAR_VALUE mClearValue;
};

// ----------- UnorderAccessTexture -------------
class UnorderAccessTexture final : public ITexture
{
public:
    UnorderAccessTexture(ID3D12Device* device, DescriptorHeap* descriptorHeap,
        UINT width, UINT height, UINT mipCount,
        DXGI_FORMAT format,
        TextureState initState = TextureState::Common);

    void BindUAV(ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor, 
        UINT depthSlice = 0, UINT mipLevel = 0);

    void BindUAV(ID3D12Device* device, DescriptorHeap* descriptorHeap);

    DescriptorData GetUavDescriptorData(UINT mipLevel = 0) const;

protected:
    bool GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState) override;
    void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) override;

private:
    
    DescriptorData mUavDescriptorData;
};