#include "stdafx.h"
#include "frameResource.h"
#include "util.h"
using namespace std;

FrameResources::FrameResources(ID3D12Device* device)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    ConstantCamera = make_unique<UploadBuffer<CameraConstant, true>>(device);
    ConstantLight = make_unique<UploadBuffer<LightConstant, true>>(device);

    ConstantObject = make_unique<UploadBuffer<ObjectConstant, true>>(device, OBJECT_MAX_SIZE);
    ConstantMaterial = make_unique<UploadBuffer<MaterialConstant, true>>(device, MATERIAL_MAX_SIZE);
}
