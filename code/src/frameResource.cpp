#include "stdafx.h"
#include "frameResource.h"
#include "util.h"
using namespace std;

FrameResources::FrameResources(ID3D12Device* device)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    ConstantRenderTargetParam = make_unique<UploadBuffer<RenderTargetParamConstant, true>>(device);
    ConstantCamera = make_unique<UploadBuffer<CameraConstant, true>>(device);
    ConstantLight = make_unique<UploadBuffer<LightConstant, true>>(device);
    ConstantShadow = make_unique<UploadBuffer<ShadowConst, true>>(device);
    ConstantShadowCaster = make_unique<UploadBuffer<ShadowCasterConstant, true>>(device, MAX_CASCADE_COUNT + MAX_SPOT_LIGHT_COUNT + MAX_POINT_LIGHT_COUNT * 6 );


    ConstantObject = make_unique<UploadBuffer<ObjectConstant, true>>(device, OBJECT_MAX_SIZE);
    ConstantMaterial = make_unique<UploadBuffer<MaterialConstant, true>>(device, MATERIAL_MAX_SIZE);
}

PassData* FrameResources::GetOrCreate(PassBase* key, std::function<std::unique_ptr<PassData>()> createFunc)
{
    auto ite = mPassResource.find(key);
    if (ite == mPassResource.end())
    {
        return (mPassResource[key] = createFunc()).get();
    }
    else
    {
        return ite->second.get();
    }
}