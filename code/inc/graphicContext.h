#pragma once
#include "stdafx.h"

class DescriptorHeap;
class ResourceContainer;
struct FrameResource;

struct GraphicContext
{
    ID3D12Device* device;
    ID3D12GraphicsCommandList* commandList;
    DescriptorHeap* descriptorHeap;
    ResourceContainer* resourceContainer;
    FrameResource* frameResource;
};