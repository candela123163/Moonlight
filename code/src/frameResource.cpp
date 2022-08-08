#include "frameResource.h"
#include "util.h"

FrameResource::FrameResource(ID3D12Device* device)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));
}