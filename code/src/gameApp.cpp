#include "stdafx.h"
#include <WindowsX.h>
#include "gameApp.h"
#include "util.h"
#include "passBase.h"
#include "debugPass.h"
#include "skyBoxPass.h"
#include "opaqueLitPass.h"
#include "IBLPreprocessPass.h"
#include "shadowPass.h"
using namespace DirectX;
using namespace std;

GameApp* GameApp::mGame = nullptr;

GameApp* GameApp::GetApp() {
    return mGame;
}

GameApp::GameApp(HINSTANCE hInstance)
    : mhAppInst(hInstance)
{
    // Only one D3DApp can be constructed.
    assert(mGame == nullptr);
    mGame = this;
}

GameApp::~GameApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

HINSTANCE GameApp::AppInst() const
{
    return mhAppInst;
}

HWND GameApp::MainWnd() const
{
    return mhMainWnd;
}

float GameApp::AspectRatio()const
{
    return static_cast<float>(mClientWidth) / mClientHeight;
}

int GameApp::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				Update();
				Draw();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool GameApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	if (!InitGraphic())
		return false;

	mScene.OnLoadOver(mGraphicContext);

	PreprocessPasses();

	return true;
}

void GameApp::SetDefaultRenderTarget()
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->OMSetRenderTargets(1, get_rvalue_ptr(CurrentBackBufferView()), true, get_rvalue_ptr(DepthStencilView()));
}

LRESULT GameApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		return 0;
		
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return GameApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

void GameApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
	mScene.OnMouseDown(btnState, x, y);
}

void GameApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
	mScene.OnMouseUp(btnState, x, y);
}

void GameApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	float dx = static_cast<float>(x - mLastMousePos.x);
	float dy = static_cast<float>(y - mLastMousePos.y);
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	mScene.OnMouseMove(btnState, x, y, dx, dy);
}

void GameApp::OnKeyboardInput()
{
	mScene.OnKeyboardInput(mTimer);
}

bool GameApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&R, style, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", WINDOW_TITLE,
		style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool GameApp::InitDirect3D()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	CreateFrameResource();
	CreateCommandObjects();
	CreateSwapChain();
	CreateDescriptorHeap();
	CreateDefaultRtvDsv();
	CreateGraphicContext();
	SetFullWindowViewPort();

	return true;
}

bool GameApp::InitGraphic()
{
	ThrowIfFailed(mCommandList->Reset(CurrentFrameResource()->CmdListAlloc.Get(), nullptr));
	
	PreparePasses();
	
	mScene.Load(Globals::ScenePath / "sponza.json", mGraphicContext);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
	
	return true;
}

void GameApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		CurrentFrameResource()->CmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}

void GameApp::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = mSwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void GameApp::CreateFrameResource()
{
	for (int i = 0; i < mFrameCount; i++)
	{
		mFrameResources.push_back(std::make_unique<FrameResources>(md3dDevice.Get()));
	}
}

void GameApp::CreateDescriptorHeap()
{
	mDescriptorHeap = std::make_unique<DescriptorHeap>(md3dDevice.Get());
}

void GameApp::CreateDefaultRtvDsv()
{
	auto descriptorData = mDescriptorHeap->GetNextnRtvDescriptor(mSwapChainBufferCount);
	mRtvDescriptorCPUStart = descriptorData.CPUHandle;
	mRtvDescriptorSize = descriptorData.IncrementSize;

	for (UINT i = 0; i < mSwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, descriptorData.CPUHandle);
		descriptorData.CPUHandle.Offset(1, mRtvDescriptorSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count =  1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
#ifdef REVERSE_Z
	optClear.DepthStencil.Depth = 0.0f;
#else
	optClear.DepthStencil.Depth = 1.0f;
#endif
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	descriptorData = mDescriptorHeap->GetNextnDsvDescriptor(1);
	mDsvDescriptorCPUStart = descriptorData.CPUHandle;
	mDsvDescriptorSize = descriptorData.IncrementSize;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mDsvDescriptorCPUStart);
}

void GameApp::SetFullWindowViewPort()
{
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

void GameApp::CreateGraphicContext()
{
	mGraphicContext = {
		md3dDevice.Get(),
		mCommandList.Get(),
		mCommandQueue.Get(),
		&mTimer,
		mDescriptorHeap.get(),
		CurrentFrameResource(),
		mBackBufferFormat,
		mDepthStencilFormat,
		mClientWidth,
		mClientHeight,

		&mScene
	};
}

void GameApp::UpdateGraphicContext()
{
	// just update current frame resource
	mGraphicContext.frameResource = CurrentFrameResource();
}

void GameApp::PreparePasses()
{
	// load pass
	mPasses.push_back(make_unique<IBLPreprocessPass>());
	mPasses.push_back(make_unique<ShadowPass>());
	mPasses.push_back(make_unique<OpaqueLitPass>());
	mPasses.push_back(make_unique<SkyboxPass>());
	//mPasses.push_back(make_unique<DebugPass>());

	for (auto& pass : mPasses) {
		pass->PreparePass(mGraphicContext);
	}
}

void GameApp::DrawPasses()
{
	for (auto& pass : mPasses) {
		pass->DrawPass(mGraphicContext);
	}
}

void GameApp::PreprocessPasses()
{
	ThrowIfFailed(CurrentFrameResource()->CmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(CurrentFrameResource()->CmdListAlloc.Get(), nullptr));

	for (auto& pass : mPasses) {
		pass->PreprocessPass(mGraphicContext);
	}

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
}

void GameApp::ReleasePasses()
{
	for (auto& pass : mPasses) {
		pass->ReleasePass(mGraphicContext);
	}
	mPasses.clear();
}

FrameResources* GameApp::CurrentFrameResource() const
{
	return mFrameResources[mCurrFrameIndex].get();
}

ID3D12Resource* GameApp::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GameApp::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvDescriptorCPUStart,
		mCurrBackBufferIndex,
		mRtvDescriptorSize
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE GameApp::DepthStencilView() const
{
	return mDsvDescriptorCPUStart;
}

void GameApp::FlushCommandQueue()
{
	mCurrentFence++;

	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void GameApp::WaitForPreFrameComplete()
{
	FrameResources* currentFrameResource = CurrentFrameResource();
	if (mFence->GetCompletedValue() < currentFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ThrowIfFailed(mFence->SetEventOnCompletion(currentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void GameApp::FlipFrame()
{	
	++mCurrentFence;
	CurrentFrameResource()->Fence = mCurrentFence;
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
	
	mCurrBackBufferIndex = (mCurrBackBufferIndex + 1) % mSwapChainBufferCount;
	mCurrFrameIndex = (mCurrFrameIndex + 1) % mFrameCount;
}

void GameApp::Update()
{
	OnKeyboardInput();
	WaitForPreFrameComplete();

	UpdateGraphicContext();
	mScene.OnUpdate(mGraphicContext);
}

void GameApp::Draw()
{
	ThrowIfFailed(CurrentFrameResource()->CmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(CurrentFrameResource()->CmdListAlloc.Get(), nullptr));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescriptorHeap->GetSrvHeap()};
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);

#ifdef REVERSE_Z
	float zClearValue = 0.0f;
#else
	float zClearValue = 1.0f;
#endif
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, zClearValue, 0, 0, nullptr);

	DrawPasses();

	mCommandList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
	
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));

	FlipFrame();
}