#pragma once
#include "stdafx.h"
#include "descriptor.h"
#include "resourceContainer.h"
#include "frameResource.h"
using Microsoft::WRL::ComPtr;

class GameApp
{
public:
    GameApp(HINSTANCE hInstance);
    GameApp(const GameApp& rhs) = delete;
    GameApp& operator=(const GameApp& rhs) = delete;
    ~GameApp();

    static GameApp* GetApp();

    HINSTANCE AppInst()const;
    HWND      MainWnd()const;
    float     AspectRatio()const;

    int Run();

    bool Initialize();
    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // handle mouse & keyboard input
    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);
    void OnKeyboardInput();

    // init window & d3d
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateFrameResource();
    void CreateDescriptorHeap();
    void CreateDefaultRtvDsv();
    void SetFullWindowViewPort();

    void FlushCommandQueue();
    void WaitForPreFrameComplete();
    void FlipFrame();

    FrameResource* CurrentFrameResource() const;
    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    void Update();
    void Draw();

private:
    static GameApp* mGame;

    HINSTANCE mhAppInst = nullptr;
    HWND      mhMainWnd = nullptr;
    bool      mAppPaused = false;

    Timer mTimer;

    ComPtr<IDXGIFactory4> mdxgiFactory;
    ComPtr<IDXGISwapChain> mSwapChain;
    ComPtr<ID3D12Device> md3dDevice;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    std::unique_ptr<DescriptorHeap> mDescriptorHeap;
    std::unique_ptr<ResourceContainer> mResourceContainer;

    static const int mSwapChainBufferCount = 2;
    int mCurrBackBufferIndex = 0;
    ComPtr<ID3D12Resource> mSwapChainBuffer[mSwapChainBufferCount];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE mRtvDescriptorCPUStart, mDsvDescriptorCPUStart;
    UINT mRtvDescriptorSize = 0, mDsvDescriptorSize = 0;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    int mCurrFrameIndex = 0;
    static const int mFrameCount = 3;

    POINT mLastMousePos;

    int mClientWidth = WINDOW_WIDTH;
    int mClientHeight = WINDOW_HEIGHT;
};
