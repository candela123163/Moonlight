#pragma once
#include "stdafx.h"
using Microsoft::WRL::ComPtr;

class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device* device);

	std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetNextnRtvDescriptor(UINT n);
	std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetNextnDsvDescriptor(UINT n);
	std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetNextnSrvDescriptor(UINT n);

	UINT GetRtvDescriptorSize() const { return mRtvDescriptorSize; }
	UINT GetDsvDescriptorSize() const { return mDsvDescriptorSize; }
	UINT GetSrvDescriptorSize() const { return mCbvSrvUavDescriptorSize; }

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