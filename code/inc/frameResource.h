#pragma once
#include "stdafx.h"

struct FrameResource
{
    FrameResource(ID3D12Device* device);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource() = default;


    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    UINT64 Fence = 0;
};