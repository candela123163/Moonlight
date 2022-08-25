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
    const DescriptorData& GetSrvDescriptorData() const { return mSrvDescriptorData; }

    UINT GetWidth() const { return mWidth; }
    UINT GetHeight() const { return mHeight; }
    UINT GetDepthCount() const { return mDepthCount; }
    UINT GetMipCount() const { return mMipCount; }
    TextureDimension GetDimension() const { return mDimension; }
        
protected:
    virtual void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) = 0;

protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> mTexture = nullptr;
    UINT mWidth = 0, mHeight = 0, mDepthCount = 1, mMipCount = 1;
    TextureDimension mDimension = TextureDimension::Tex2D;
    DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
    DescriptorData mSrvDescriptorData;
};


// ----------- Texture -------------
class Texture final : public ITexture
{
public:
    Texture(ID3D12Device* device, 
        DescriptorHeap* descriptorHeap,
        ID3D12CommandQueue* commandQueue,
        const std::filesystem::path& filePath);

    static Texture* GetOrLoad(const std::filesystem::path& texturePath, const GraphicContext& context);

protected:
    void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) override;
};


// ----------- RenderTexture -------------
enum class RenderTextureUsage
{
    ColorBuffer = 0,
    DepthBuffer = 1
};

enum class RenderTextureState
{
    Write = 0,
    Read = 1,
    Common = 2
};

class RenderTexture final : public ITexture
{
public:
    RenderTexture(ID3D12Device* device, DescriptorHeap* descriptorHeap,
        UINT width, UINT height,
        UINT depthCount, UINT mipCount, TextureDimension dimension,
        RenderTextureUsage usage, DXGI_FORMAT format,
        RenderTextureState initState = RenderTextureState::Common);

    void BindRTV(ID3D12Device* device, 
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
        UINT depthSlice = 0, UINT mipLevel = 0);

    void BindRTV(ID3D12Device* device, DescriptorHeap* descriptorHeap);

    void TransitionTo(ID3D12GraphicsCommandList* commandList, RenderTextureState targetState);

    void SetViewPort(ID3D12GraphicsCommandList* commandList, UINT mipLevel = 0);

    void SetAsRenderTarget(ID3D12GraphicsCommandList* commandList,
        UINT depthSlice, UINT mipLevel,
        UINT otherRTVNum = 0, D3D12_CPU_DESCRIPTOR_HANDLE* otherRTVHandles = nullptr,
        D3D12_CPU_DESCRIPTOR_HANDLE* otherDSVHandles = nullptr
    );

private:
    void GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) override;
    D3D12_RESOURCE_STATES GetD3DState(RenderTextureState state);

private:
    RenderTextureState mCurrState;
    RenderTextureUsage mUsage;
    DescriptorData mRtvDescriptorData;
};