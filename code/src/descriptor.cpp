#include "stdafx.h"
#include "descriptor.h"
#include "util.h"
#include "globals.h"
using namespace std;

DescriptorHeap::DescriptorHeap(ID3D12Device* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = HEAP_COUNT_RTV;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = HEAP_COUNT_DSV;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(device->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = HEAP_COUNT_SRV;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));

    mRtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

DescriptorData  DescriptorHeap::GetNextnRtvDescriptor(UINT n)
{
    assert(n > 0 && mRtvIndex + n <= HEAP_COUNT_RTV);
    UINT rtvIndex = mRtvIndex;
    mRtvIndex += n;
    return {
        CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), rtvIndex, mRtvDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(mRtvHeap->GetGPUDescriptorHandleForHeapStart(), rtvIndex, mRtvDescriptorSize),
        mRtvDescriptorSize,
        rtvIndex
    };
}

DescriptorData  DescriptorHeap::GetNextnDsvDescriptor(UINT n)
{
    assert(n > 0 && mDsvIndex + n <= HEAP_COUNT_DSV);
    UINT dsvIndex = mDsvIndex;
    mDsvIndex += n;
    return {
        CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), dsvIndex, mDsvDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(mDsvHeap->GetGPUDescriptorHandleForHeapStart(), dsvIndex, mDsvDescriptorSize),
        mDsvDescriptorSize,
        dsvIndex
    };
}

DescriptorData  DescriptorHeap::GetNextnSrvDescriptor(UINT n)
{
    assert(n > 0 && mSrvIndex + n <= HEAP_COUNT_SRV);
    UINT srvIndex = mSrvIndex;
    mSrvIndex += n;
    return {
        CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart(), srvIndex, mCbvSrvUavDescriptorSize),
        CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvHeap->GetGPUDescriptorHandleForHeapStart(), srvIndex, mCbvSrvUavDescriptorSize),
        mCbvSrvUavDescriptorSize,
        srvIndex
    };
}