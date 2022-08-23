#include "stdafx.h"
#include "texture.h"
#include "util.h"
#include "descriptor.h"
#include "globals.h"
using namespace std;
using namespace DirectX;

// ----------- ITexture -------------
void ITexture::BindSRV(ID3D12Device* device, DescriptorHeap* descriptorHeap)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    GetSRVDes(SRVDesc);
    DescriptorData descriptorData = descriptorHeap->GetNextnSrvDescriptor(1);
    mSRVHeapIndex = descriptorData.HeapIndex;
    mGpuDescriptor = descriptorData.GPUHandle;
    device->CreateShaderResourceView(mTexture.Get(), &SRVDesc, descriptorData.CPUHandle);
}

void ITexture::BindSRV(ID3D12Device* device, const DescriptorData& descriptorData)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    GetSRVDes(SRVDesc);
    mSRVHeapIndex = descriptorData.HeapIndex;
    device->CreateShaderResourceView(mTexture.Get(), &SRVDesc, descriptorData.CPUHandle);
    mGpuDescriptor = descriptorData.GPUHandle;
}

// ----------- Texture -------------
Texture::Texture(ID3D12Device* device, 
    DescriptorHeap* srvHeap,
    ID3D12CommandQueue* commandQueue, 
    const filesystem::path& filePath)
{
    ResourceUploadBatch uploadBatch(device);
    uploadBatch.Begin();

    string extension = filePath.extension().string();
    bool isCube = false;

    if (extension == ".dds")
    {    
        CreateDDSTextureFromFile(device, uploadBatch, filePath.wstring().c_str(), mTexture.GetAddressOf(), true, 0, nullptr, &isCube);
    }
    else
    {
        CreateWICTextureFromFile(device, uploadBatch, filePath.wstring().c_str(), mTexture.GetAddressOf(), true);
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

    BindSRV(device, srvHeap);
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

Texture* Texture::GetOrLoad(const filesystem::path& texturePath, const GraphicContext& context)
{
    string textureName = texturePath.filename().string();
    size_t textureKey = hash<string>()(textureName);

    if (!Globals::TextureContainer.Contains(textureKey))
    {
        Globals::TextureContainer.Insert(
            textureKey,
            make_unique<Texture>(context.device, context.descriptorHeap, context.commandQueue, texturePath)
        );
    }

    return Globals::TextureContainer.Get(textureKey);
}

// ----------- RenderTexture -------------
RenderTexture::RenderTexture(ID3D12Device* device,
    UINT width, UINT height,
    UINT depthCount, UINT mipCount, TextureDimension dimension,
    RenderTextureUsage usage, DXGI_FORMAT format,
    RenderTextureState initState)
{
    mWidth = width;
    mHeight = height;
    mDepthCount = depthCount;
    mMipCount = mipCount;
    mDimension = dimension;
    mUsage = usage;
    mFormat = format;
    mCurrState = initState;

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

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mFormat;

    if (mUsage == RenderTextureUsage::ColorBuffer)
    {
        optClear.Color[0] = 0.0f;
        optClear.Color[1] = 0.0f;
        optClear.Color[2] = 0.0f;
        optClear.Color[3] = 0.0f;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else
    {
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        optClear.DepthStencil.Depth = 1.0f;
        optClear.DepthStencil.Stencil = 0;
    }
    
    ThrowIfFailed(device->CreateCommittedResource(
        get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        GetD3DState(initState),
        &optClear,
        IID_PPV_ARGS(mTexture.GetAddressOf())));
}

D3D12_RESOURCE_STATES RenderTexture::GetD3DState(RenderTextureState state)
{
    D3D12_RESOURCE_STATES d3dState;
    switch (state)
    {
    case RenderTextureState::Write:
        d3dState = D3D12_RESOURCE_STATE_RENDER_TARGET;
        break;
    case RenderTextureState::Read:
        d3dState = mUsage == RenderTextureUsage::ColorBuffer ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_DEPTH_READ;
        break;
    case RenderTextureState::Common:
        d3dState = D3D12_RESOURCE_STATE_COMMON;
        break;
    default:
        break;
    }

    return d3dState;
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
        switch (mDimension)
        {
        case TextureDimension::Tex2D:
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Format = mFormat;
            break;

        case TextureDimension::CubeMap:
        case TextureDimension::Tex2DArray:
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
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

void RenderTexture::TransitionTo(ID3D12GraphicsCommandList* commandList, RenderTextureState targetState)
{
    if (targetState != mCurrState)
    {
        commandList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(mTexture.Get(), GetD3DState(mCurrState), GetD3DState(targetState))));
        mCurrState = targetState;
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