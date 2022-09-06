#pragma once
#include "stdafx.h"
#include "descriptor.h"
#include "frameResource.h"
#include "globals.h"
#include "scene.h"
using Microsoft::WRL::ComPtr;

class PassBase;
class RenderTexture;

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

public:
    void SetDefaultRenderTarget(bool writeColor = true, bool writeDepth = true);
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    RenderTexture* GetDepthStencilTarget() const;

private:
    // handle mouse & keyboard input
    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);
    void OnKeyboardInput();

    // init window & d3d
    bool InitMainWindow();
    bool InitDirect3D();
    bool InitGraphic();

    void CreateCommandObjects();
    void CreateSwapChain();
    void CreateFrameResource();
    void CreateDescriptorHeap();
    void CreateDefaultRtvDsv();
    void SetFullWindowViewPort();
    void CreateGraphicContext();
    void UpdateGraphicContext();
    
    void PreparePasses();
    void PreprocessPasses();
    void DrawPasses();
    void ReleasePasses();

    void FlushCommandQueue();
    void WaitForPreFrameComplete();
    void FlipFrame();

    FrameResources* CurrentFrameResource() const;
    ID3D12Resource* CurrentBackBuffer() const;

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
    

    static const int mSwapChainBufferCount = SWAP_CHAIN_COUNT;
    int mCurrBackBufferIndex = 0;
    ComPtr<ID3D12Resource> mSwapChainBuffer[mSwapChainBufferCount];
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    DescriptorData mRtvDescriptorData; 

    std::unique_ptr<RenderTexture> mDepthStencilTarget;


    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    std::vector<std::unique_ptr<FrameResources>> mFrameResources;
    int mCurrFrameIndex = 0;
    static const int mFrameCount = FRAME_COUNT;

    POINT mLastMousePos;

    int mClientWidth = WINDOW_WIDTH;
    int mClientHeight = WINDOW_HEIGHT;

    GraphicContext mGraphicContext;

    Scene mScene;

    // pass
    std::vector<std::unique_ptr<PassBase>> mPasses;
};
