#include "stdafx.h"
#include "texture.h"
#include "util.h"
#include "globals.h"
using namespace std;
using namespace DirectX;

// ----------- ITexture -------------
void ITexture::BindSRV(ID3D12Device* device, DescriptorHeap* descriptorHeap)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    GetSRVDes(SRVDesc);
    mSrvDescriptorData = descriptorHeap->GetNextnSrvDescriptor(1);
    device->CreateShaderResourceView(mTexture.Get(), &SRVDesc, mSrvDescriptorData.CPUHandle);
}

void ITexture::BindSRV(ID3D12Device* device, const DescriptorData& descriptorData)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    GetSRVDes(SRVDesc);
    mSrvDescriptorData = descriptorData;
    device->CreateShaderResourceView(mTexture.Get(), &SRVDesc, descriptorData.CPUHandle);
}

bool ITexture::TransitionTo(ID3D12GraphicsCommandList* commandList, TextureState targetState)
{
    bool allSucceed = true;
    for (size_t depthSlice = 0; depthSlice < mDepthCount; depthSlice++)
    {
        for (size_t mipSlice = 0; mipSlice < mMipCount; mipSlice++)
        {
            allSucceed &= TransitionSubResourceTo(commandList, depthSlice, mipSlice, targetState);
        }
    }
    return allSucceed;
}

bool ITexture::CopyResource(ID3D12GraphicsCommandList* commandList, ITexture& copySource)
{
    if (!(GetWidth() == copySource.GetWidth() &&
        GetHeight() == copySource.GetHeight() &&
        GetFormat() == copySource.GetFormat()) &&
        GetDepthCount() == copySource.GetDepthCount() &&
        GetMipCount() == copySource.GetMipCount()
        )
    {
        return false;
    }

    if (TransitionTo(commandList, TextureState::CopyDest) &&
        copySource.TransitionTo(commandList, TextureState::CopySrouce))
    {
        commandList->CopyResource(GetResource(), copySource.GetResource());
        return true;
    }
    return false;
}

bool ITexture::TransitionSubResourceTo(ID3D12GraphicsCommandList* commandList, UINT depthSlice, UINT mipSlice, TextureState targetState)
{
    UINT subResourceIdx = depthSlice * mMipCount + mipSlice;
    TextureState& currState = mSubResourceState[subResourceIdx];
    if (targetState != currState)
    {
        D3D12_RESOURCE_STATES current, target;
        if (GetD3DState(currState, current) && GetD3DState(targetState, target)) {
            commandList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(mTexture.Get(), current, target, subResourceIdx)));
            currState = targetState;
            return true;
        }
        return false;
    }
    return true;
}

// ----------- Texture -------------
Texture::Texture(ID3D12Device* device, 
    DescriptorHeap* descriptorHeap,
    ID3D12CommandQueue* commandQueue, 
    const filesystem::path& filePath,
    bool sRGB)
{
    ResourceUploadBatch uploadBatch(device);
    uploadBatch.Begin();

    string extension = filePath.extension().string();
    bool isCube = false;

    if (extension == ".dds")
    {   
        DDS_LOADER_FLAGS sRGBFlag = sRGB ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_IGNORE_SRGB;
        CreateDDSTextureFromFileEx(device, uploadBatch, filePath.wstring().c_str(),  0, D3D12_RESOURCE_FLAG_NONE, sRGBFlag, mTexture.GetAddressOf(), nullptr, &isCube);
    }
    else
    {
        WIC_LOADER_FLAGS sRGBFlag = sRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_IGNORE_SRGB;
        sRGBFlag |= WIC_LOADER_MIP_AUTOGEN;
        CreateWICTextureFromFileEx(device, uploadBatch, filePath.wstring().c_str(), 0, D3D12_RESOURCE_FLAG_NONE, sRGBFlag, mTexture.GetAddressOf());
    }

    auto uploadOperation = uploadBatch.End(commandQueue);
    uploadOperation.wait();

    D3D12_RESOURCE_DESC desc = mTexture->GetDesc();
    mWidth = desc.Width;
    mHeight = desc.Height;
    mDepthCount = desc.DepthOrArraySize;
    mMipCount = desc.MipLevels;
    mFormat = desc.Format;
    mDimension = isCube ? TextureDimension::CubeMap : (mDepthCount > 1 ? TextureDimension::Tex2DArray : TextureDimension::Tex2D);

    mSubResourceState.assign(mDepthCount * mMipCount, TextureState::Read);

    BindSRV(device, descriptorHeap);
}

void Texture::GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc)
{
    outDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    outDesc.Format = mFormat;
    switch (mDimension)
    {
    case TextureDimension::Tex2D:
        outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        outDesc.Texture2D.MostDetailedMip = 0;
        outDesc.Texture2D.MipLevels = mMipCount;
        outDesc.Texture2D.PlaneSlice = 0;
        outDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        break;
    case TextureDimension::CubeMap:
        outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        outDesc.TextureCube.MostDetailedMip = 0;
        outDesc.TextureCube.MipLevels = mMipCount;
        outDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        break;
    case TextureDimension::Tex2DArray:
        outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        outDesc.Texture2DArray.MostDetailedMip = 0;
        outDesc.Texture2DArray.MipLevels = mMipCount;
        outDesc.Texture2DArray.PlaneSlice = 0;
        outDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        outDesc.Texture2DArray.ArraySize = mDepthCount;
        outDesc.Texture2DArray.FirstArraySlice = 0;
        break;
    default:
        break;
    }
}

Texture* Texture::GetOrLoad(const filesystem::path& texturePath, bool sRGB, const GraphicContext& context)
{
    string textureName = texturePath.filename().string();
    size_t textureKey = hash<string>()(textureName);

    if (!Globals::TextureContainer.Contains(textureKey))
    {
        return Globals::TextureContainer.Insert(
            textureKey,
            make_unique<Texture>(context.device, context.descriptorHeap, context.commandQueue, texturePath, sRGB)
        );
    }

    return Globals::TextureContainer.Get(textureKey);
}

bool Texture::GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState)
{
    assert(state != TextureState::Write);
    assert(state != TextureState::CopyDest);

    switch (state)
    {
    case TextureState::Read:
        outState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        break;
    case TextureState::Common:
        outState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case TextureState::CopySrouce:
        outState = D3D12_RESOURCE_STATE_COPY_SOURCE;
        break;
    default:
        return false;
    }
    return true;
}


// ----------- RenderTexture -------------
RenderTexture::RenderTexture(ID3D12Device* device, DescriptorHeap* descriptorHeap,
    UINT width, UINT height,
    UINT depthCount, UINT mipCount, TextureDimension dimension,
    RenderTextureUsage usage, DXGI_FORMAT format,
    TextureState initState, const D3D12_CLEAR_VALUE* clearValue)
{
    mWidth = width;
    mHeight = height;
    mDepthCount = depthCount;
    mMipCount = mipCount;
    mDimension = dimension;
    mUsage = usage;
    mFormat = format;

    mSubResourceState.assign(depthCount * mipCount, initState);

    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = mDepthCount;
    texDesc.MipLevels = mMipCount;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = mUsage == RenderTextureUsage::ColorBuffer ? 
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    if (clearValue)
    {
        mClearValue = *clearValue;
        mClearValue.Format = mFormat;
    }
    else {
        mClearValue.Format = mFormat;

        if (mUsage == RenderTextureUsage::ColorBuffer)
        {
            mClearValue.Color[0] = 0.0f;
            mClearValue.Color[1] = 0.0f;
            mClearValue.Color[2] = 0.0f;
            mClearValue.Color[3] = 0.0f;
        }
        else
        {
#ifdef REVERSE_Z
            mClearValue.DepthStencil.Depth = 0.0f;
#else
            mClearValue.DepthStencil.Depth = 1.0f;
#endif
            mClearValue.DepthStencil.Stencil = 0;
    }
}
    D3D12_RESOURCE_STATES state;
    GetD3DState(initState, state);
    ThrowIfFailed(device->CreateCommittedResource(
        get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        state,
        &mClearValue,
        IID_PPV_ARGS(mTexture.GetAddressOf())));
    
    BindRTV(device, descriptorHeap);
    BindSRV(device, descriptorHeap);
}

bool RenderTexture::GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState)
{

    switch (state)
    {
    case TextureState::Write:
        outState = mUsage == RenderTextureUsage::ColorBuffer ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_DEPTH_WRITE;
        break;
    case TextureState::Read:
        outState = mUsage == RenderTextureUsage::ColorBuffer ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_DEPTH_READ;
        break;
    case TextureState::Common:
        outState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case TextureState::CopyDest:
        outState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
    case TextureState::CopySrouce:
        outState = D3D12_RESOURCE_STATE_COPY_SOURCE;
        break;

    default:
        return false;
    }

    return true;
}

void RenderTexture::GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc)
{
    outDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    switch (mFormat)
    {
    case DXGI_FORMAT_D16_UNORM:
        outDesc.Format = DXGI_FORMAT_R16_UNORM;
        break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        outDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT:
        outDesc.Format = DXGI_FORMAT_R32_FLOAT;
        break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        outDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        break;
    default:
        outDesc.Format = mFormat;
    }
    switch (mDimension)
    {
    case TextureDimension::CubeMap:
        outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        outDesc.TextureCube.MostDetailedMip = 0;
        outDesc.TextureCube.MipLevels = mMipCount;
        outDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        break;
    case TextureDimension::Tex2D:
        outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        outDesc.Texture2D.MostDetailedMip = 0;
        outDesc.Texture2D.MipLevels = mMipCount;
        outDesc.Texture2D.PlaneSlice = 0;
        outDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        break;
    case TextureDimension::Tex2DArray:
        outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        outDesc.Texture2DArray.MostDetailedMip = 0;
        outDesc.Texture2DArray.MipLevels = mMipCount;
        outDesc.Texture2DArray.PlaneSlice = 0;
        outDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        outDesc.Texture2DArray.ArraySize = mDepthCount;
        outDesc.Texture2DArray.FirstArraySlice = 0;
        break;
    }
}

void RenderTexture::BindRTV(ID3D12Device* device,
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
    UINT depthSlice, UINT mipLevel)
{
    if (mUsage == RenderTextureUsage::ColorBuffer)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
        switch (mDimension)
        {
        case TextureDimension::Tex2D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Format = mFormat;
            rtvDesc.Texture2D.PlaneSlice = 0;
            rtvDesc.Texture2D.MipSlice = mipLevel;
            break;

        case TextureDimension::CubeMap:
        case TextureDimension::Tex2DArray:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Format = mFormat;
            rtvDesc.Texture2DArray.PlaneSlice = 0;
            rtvDesc.Texture2DArray.ArraySize = 1;
            rtvDesc.Texture2DArray.FirstArraySlice = depthSlice;
            rtvDesc.Texture2DArray.MipSlice = mipLevel;
            break;

        default:
            break;
        }
        device->CreateRenderTargetView(mTexture.Get(), &rtvDesc, descriptor);
    }
    else
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Format = mFormat;

        switch (mDimension)
        {
        case TextureDimension::Tex2D:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = mipLevel;
            break;

        case TextureDimension::CubeMap:
        case TextureDimension::Tex2DArray:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.ArraySize = 1;
            dsvDesc.Texture2DArray.MipSlice = mipLevel;
            dsvDesc.Texture2DArray.FirstArraySlice = depthSlice;

            break;
        default:
            break;
        }
        device->CreateDepthStencilView(mTexture.Get(), &dsvDesc, descriptor);
    }
}

void RenderTexture::BindRTV(ID3D12Device* device, DescriptorHeap* descriptorHeap)
{
    if (mUsage == RenderTextureUsage::ColorBuffer)
    {
        mRtvDescriptorData = descriptorHeap->GetNextnRtvDescriptor(mMipCount * mDepthCount);
    }
    else
    {
        mRtvDescriptorData = descriptorHeap->GetNextnDsvDescriptor(mMipCount * mDepthCount);
    }
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = mRtvDescriptorData.CPUHandle;
    for (size_t i = 0; i < mDepthCount; i++)
    {
        for (size_t j = 0; j < mMipCount; j++)
        {
            BindRTV(device, cpuHandle, i, j);
            cpuHandle.Offset(mRtvDescriptorData.IncrementSize);
        }
    }
}

void RenderTexture::SetViewPort(ID3D12GraphicsCommandList* commandList, UINT mipLevel)
{
    UINT currentWidth = mWidth >> mipLevel;
    UINT currentHeight = mHeight >> mipLevel;
    D3D12_VIEWPORT Viewport({ 0.0f, 0.0f, (float)(currentWidth), (float)(currentHeight), 0.0f, 1.0f });
    D3D12_RECT ScissorRect({ 0, 0, (LONG)currentWidth, (LONG)currentHeight });
    commandList->RSSetViewports(1, &Viewport);
    commandList->RSSetScissorRects(1, &ScissorRect);
}

void RenderTexture::SetAsRenderTarget(ID3D12GraphicsCommandList* commandList,
    UINT depthSlice, UINT mipLevel,
    UINT otherRTVNum, const D3D12_CPU_DESCRIPTOR_HANDLE* otherRTVHandles,
    const D3D12_CPU_DESCRIPTOR_HANDLE* otherDSVHandles
)
{
    UINT rtvNum = 0;
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles = nullptr, * dsvHandles = nullptr;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = mRtvDescriptorData.CPUHandle;
    cpuHandle.Offset(mMipCount * depthSlice + mipLevel, mRtvDescriptorData.IncrementSize);

    if (mUsage == RenderTextureUsage::DepthBuffer)
    {
        rtvNum = otherRTVNum;
        rtvHandles = otherRTVHandles;
        dsvHandles = &cpuHandle;
    }
    else
    {
        rtvNum = 1;
        rtvHandles = &cpuHandle;
        dsvHandles = otherDSVHandles;
    }

    SetViewPort(commandList, mipLevel);
    commandList->OMSetRenderTargets(rtvNum, rtvHandles, false, dsvHandles);
}

void RenderTexture::SetAsRenderTarget(ID3D12GraphicsCommandList* commandList,
    UINT depthSlice, UINT mipLevel,
    const RenderTexture& other, UINT otherDepthSlice, UINT otherMipLevel
)
{
    UINT otherRTVNum = 0;
    const D3D12_CPU_DESCRIPTOR_HANDLE* otherRTVHandles = nullptr;
    const D3D12_CPU_DESCRIPTOR_HANDLE* otherDSVHandles = nullptr;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = other.GetRtvDescriptorData().CPUHandle;
    cpuHandle.Offset(other.GetMipCount() * otherDepthSlice + otherMipLevel, mRtvDescriptorData.IncrementSize);

    if (other.GetUsage() == RenderTextureUsage::ColorBuffer)
    {
        otherRTVNum = 1;
        otherRTVHandles = &cpuHandle;
    }
    else
    {
        otherDSVHandles = &cpuHandle;
    }

    SetAsRenderTarget(commandList, depthSlice, mipLevel, otherRTVNum, otherRTVHandles, otherDSVHandles);
}

void RenderTexture::Clear(ID3D12GraphicsCommandList* commandList, UINT depthSlice, UINT mipLevel)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = mRtvDescriptorData.CPUHandle;
    cpuHandle.Offset(mMipCount * depthSlice + mipLevel, mRtvDescriptorData.IncrementSize);

    if (mUsage == RenderTextureUsage::DepthBuffer)
    {
        commandList->ClearDepthStencilView(cpuHandle,
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
            mClearValue.DepthStencil.Depth, mClearValue.DepthStencil.Stencil, 
            0, nullptr);
    }
    else
    {
        commandList->ClearRenderTargetView(cpuHandle, mClearValue.Color, 0, nullptr);
    }
}

// ----------- UnorderAccessTexture -------------
UnorderAccessTexture::UnorderAccessTexture(ID3D12Device* device, DescriptorHeap* descriptorHeap,
    UINT width, UINT height, UINT mipCount,
    DXGI_FORMAT format,
    TextureState initState)
{
    mWidth = width;
    mHeight = height;
    mDepthCount = 1;
    mMipCount = mipCount;
    mFormat = format;

    mSubResourceState.assign(mDepthCount * mMipCount, initState);

    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mWidth;
    texDesc.Height = mHeight;
    texDesc.DepthOrArraySize = mDepthCount;
    texDesc.MipLevels = mMipCount;
    texDesc.Format = mFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_STATES state;
    GetD3DState(initState, state);
    ThrowIfFailed(device->CreateCommittedResource(
        get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        state,
        nullptr,
        IID_PPV_ARGS(&mTexture)));

    BindUAV(device, descriptorHeap);
    BindSRV(device, descriptorHeap);
}

void UnorderAccessTexture::BindUAV(ID3D12Device* device,
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor, 
    UINT depthSlice, UINT mipLevel )
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = mipLevel;
    uavDesc.Texture2D.PlaneSlice = depthSlice;

    device->CreateUnorderedAccessView(mTexture.Get(), nullptr, &uavDesc, descriptor);
}

void UnorderAccessTexture::BindUAV(ID3D12Device* device, DescriptorHeap* descriptorHeap)
{
    mUavDescriptorData = descriptorHeap->GetNextnSrvDescriptor(mDepthCount * mMipCount);

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle = mUavDescriptorData.CPUHandle;
    for (size_t i = 0; i < mDepthCount; i++)
    {
        for (size_t j = 0; j < mMipCount; j++)
        {
            BindUAV(device, cpuHandle, i, j);
            cpuHandle.Offset(mUavDescriptorData.IncrementSize);
        }
    }

    BindUAV(device, mUavDescriptorData.CPUHandle);
}

bool UnorderAccessTexture::GetD3DState(TextureState state, D3D12_RESOURCE_STATES& outState)
{
    switch (state)
    {
    case TextureState::Write:
        outState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        break;
    case TextureState::Read:
        outState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;
    case TextureState::Common:
        outState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case TextureState::CopySrouce:
        outState = D3D12_RESOURCE_STATE_COPY_SOURCE;
        break;
    case TextureState::CopyDest:
        outState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
    default:
        return false;
        break;
    }
    return true;
}

void UnorderAccessTexture::GetSRVDes(D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc)
{
    outDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    outDesc.Format = mFormat;
    outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    outDesc.Texture2D.MostDetailedMip = 0;
    outDesc.Texture2D.MipLevels = mMipCount;
    outDesc.Texture2D.PlaneSlice = 0;
    outDesc.Texture2D.ResourceMinLODClamp = 0.0f;
}

DescriptorData UnorderAccessTexture::GetUavDescriptorData(UINT mipLevel ) const
{
    return DescriptorData{
        CD3DX12_CPU_DESCRIPTOR_HANDLE(mUavDescriptorData.CPUHandle, mipLevel, mUavDescriptorData.IncrementSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(mUavDescriptorData.GPUHandle, mipLevel, mUavDescriptorData.IncrementSize),
        mUavDescriptorData.IncrementSize,
        mUavDescriptorData.HeapIndex + mipLevel
    };
}