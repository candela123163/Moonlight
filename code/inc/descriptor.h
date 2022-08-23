#pragma once
#include "stdafx.h"
using Microsoft::WRL::ComPtr;


struct DescriptorData
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	UINT IncrementSize;
	UINT HeapIndex;
};


class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device* device);

	DescriptorData GetNextnRtvDescriptor(UINT n);
	DescriptorData GetNextnDsvDescriptor(UINT n);
	DescriptorData GetNextnSrvDescriptor(UINT n);

	UINT GetRtvDescriptorSize() const { return mRtvDescriptorSize; }
	UINT GetDsvDescriptorSize() const { return mDsvDescriptorSize; }
	UINT GetSrvDescriptorSize() const { return mCbvSrvUavDescriptorSize; }

	ID3D12DescriptorHeap* GetRtvHeap() const { return mRtvHeap.Get(); }
	ID3D12DescriptorHeap* GetDsvHeap() const { return mDsvHeap.Get(); }
	ID3D12DescriptorHeap* GetSrvHeap() const { return mSrvHeap.Get(); }

private:
	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	UINT mRtvIndex = 0;
	UINT mDsvIndex = 0;
	UINT mSrvIndex = 0;

	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	
};