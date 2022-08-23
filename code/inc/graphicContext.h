#pragma once


struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandQueue;
class Timer;
class DescriptorHeap;
struct FrameResources;
enum DXGI_FORMAT;
class Scene;


struct GraphicContext
{
    ID3D12Device* device;
    ID3D12GraphicsCommandList* commandList;
    ID3D12CommandQueue* commandQueue;
    Timer* timer;
    DescriptorHeap* descriptorHeap;
    FrameResources* frameResource;
    DXGI_FORMAT backBufferFormat;
    DXGI_FORMAT depthStencilFormat;
    int screenWidth;
    int screenHeight;

    Scene* scene;
};

