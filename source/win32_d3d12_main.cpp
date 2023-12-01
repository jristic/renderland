
// System headers
#include <windows.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <dxgiformat.h>

// External headers
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "DirectXTex/DirectXTex.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

// Imgui example backend
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#ifdef _DEBUG
	#include <dxgidebug.h>
#endif

// Project headers
#include "types.h"
#include "math.h"
#include "matrix.h"
#include "assert.h"
#include "config.h"
#include "fileio.h"
#include "d3d12/gfx.h"
#include "rlf/rlf.h"
#include "rlf/rlfparser.h"
#include "rlf/rlfinterpreter.h"
#include "rlf/shaderparser.h"
#include "gui.h"
#include "main.h"


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


#define SafeRelease(ref) do { if (ref) { (ref)->Release(); (ref) = nullptr; } } while (0);

void CheckHresult(HRESULT hr, const char* desc)
{
	Assert(hr == S_OK, "Failed to create %s, hr=%x", desc, hr);
}

void SetupBackBuffer(gfx::Context* ctx)
{
	for (UINT i = 0; i < gfx::Context::NUM_BACK_BUFFERS; i++)
	{
		ID3D12Resource* pBackBuffer = NULL;
		ctx->SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
		ctx->Device->CreateRenderTargetView(pBackBuffer, NULL, ctx->BackBufferDescriptor[i]);
		ctx->BackBufferResource[i] = pBackBuffer;
	}
}

void CleanupBackBuffer(gfx::Context* ctx)
{
	for (u32 i = 0; i < gfx::Context::NUM_BACK_BUFFERS; i++)
		SafeRelease(ctx->BackBufferResource[i]);
}

void WaitForLastSubmittedFrame(gfx::Context* ctx)
{
	if (ctx->FrameIndex == 0)
		return;
	
	gfx::Context::Frame* frameCtx = 
		&ctx->FrameContexts[(ctx->FrameIndex-1) % gfx::Context::NUM_FRAMES_IN_FLIGHT];

	u64 fenceValue = frameCtx->FenceValue;
	if (fenceValue == 0)
		return; // No fence was signaled

	frameCtx->FenceValue = 0;
	if (ctx->Fence->GetCompletedValue() >= fenceValue)
		return;

	ctx->Fence->SetEventOnCompletion(fenceValue, ctx->FenceEvent);
	WaitForSingleObject(ctx->FenceEvent, INFINITE);
}


ImTextureID RetrieveDisplayTextureID(main::State* s)
{
	gfx::Context* ctx = s->GfxCtx;
	
	D3D12_GPU_DESCRIPTOR_HANDLE DisplaySrvGpuHnd = gfx::GetGPUDescriptor(
		&ctx->CbvSrvUavHeap, gfx::Context::RLF_RESERVED_SHADER_VIS_SLOT_INDEX);

	if (s->RlfDisplayTex.Resource == nullptr || s->PrevDisplaySize != s->DisplaySize)
	{
		WaitForLastSubmittedFrame(ctx);
		
		SafeRelease(s->RlfDisplayTex.Resource);
		SafeRelease(s->RlfDepthStencilTex.Resource);

		// create display texture
		{
			D3D12_RESOURCE_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Width = s->DisplaySize.x;
			desc.Height = s->DisplaySize.y;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Alignment = 0;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | 
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_HEAP_PROPERTIES defaultProperties;
			defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			defaultProperties.CreationNodeMask = 0;
			defaultProperties.VisibleNodeMask = 0;

			HRESULT hr = ctx->Device->CreateCommittedResource(&defaultProperties, 
				D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, 
				IID_PPV_ARGS(&s->RlfDisplayTex.Resource));
			s->RlfDisplayTex.State = D3D12_RESOURCE_STATE_RENDER_TARGET;
			s->RlfDisplayTex.Resource->SetName(L"RlfDisplayTex");
			Assert(hr == S_OK, "failed to create texture, hr=%x", hr);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE DisplaySrvCpuHnd = gfx::GetCPUDescriptor(
			&ctx->CbvSrvUavHeap, gfx::Context::RLF_RESERVED_SHADER_VIS_SLOT_INDEX);

		// display texture srv
		{
			s->RlfDisplaySrv = gfx::GetCPUDescriptor(&ctx->CbvSrvUavCreationHeap,
				gfx::Context::RLF_RESERVED_SRV_SLOT_INDEX);
			ctx->Device->CreateShaderResourceView(s->RlfDisplayTex.Resource, nullptr, 
				s->RlfDisplaySrv);

			ctx->Device->CopyDescriptorsSimple(1, DisplaySrvCpuHnd, s->RlfDisplaySrv,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		//display texture uav
		{
			s->RlfDisplayUav = gfx::GetCPUDescriptor(&ctx->CbvSrvUavCreationHeap,
				gfx::Context::RLF_RESERVED_UAV_SLOT_INDEX);
			ctx->Device->CreateUnorderedAccessView(s->RlfDisplayTex.Resource, nullptr, nullptr,
				s->RlfDisplayUav);
		}
		// display texture rtv
		{
			s->RlfDisplayRtv = GetCPUDescriptor(&ctx->RtvHeap, 
				gfx::Context::RLF_RESERVED_RTV_SLOT_INDEX);
			ctx->Device->CreateRenderTargetView(s->RlfDisplayTex.Resource, nullptr, 
				s->RlfDisplayRtv);
		}

		// create depth stencil
		{
			D3D12_RESOURCE_DESC desc = {};
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			desc.Width = s->DisplaySize.x;
			desc.Height = s->DisplaySize.y;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Alignment = 0;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			D3D12_HEAP_PROPERTIES defaultProperties;
			defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			defaultProperties.CreationNodeMask = 0;
			defaultProperties.VisibleNodeMask = 0;

			HRESULT hr = ctx->Device->CreateCommittedResource(&defaultProperties,
				D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, 
				IID_PPV_ARGS(&s->RlfDepthStencilTex.Resource));
			s->RlfDepthStencilTex.State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			s->RlfDepthStencilTex.Resource->SetName(L"RlfDepthStencilTex");
			Assert(hr == S_OK, "failed to create texture, hr=%x", hr);
		}
		// depth stencil view
		{
			s->RlfDepthStencilView = GetCPUDescriptor(&ctx->DsvHeap, 
				gfx::Context::RLF_RESERVED_DSV_SLOT_INDEX);
			ctx->Device->CreateDepthStencilView(s->RlfDepthStencilTex.Resource, nullptr,
				s->RlfDepthStencilView);
		}
	}

	return (ImTextureID)DisplaySrvGpuHnd.ptr;
}

void OnBeforeUnload(main::State* s)
{
	WaitForLastSubmittedFrame(s->GfxCtx);
}

bool CheckD3DValidation(gfx::Context* ctx, std::string& outMessage)
{
	u64 num = ctx->InfoQueue->GetNumStoredMessages();
	for (u32 i = 0 ; i < num ; ++i)
	{
		size_t messageLength;
		HRESULT hr = ctx->InfoQueue->GetMessage(i, nullptr, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		D3D12_MESSAGE* message = (D3D12_MESSAGE*)malloc(messageLength);
		ctx->InfoQueue->GetMessage(i, message, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		outMessage += message->pDescription;
		outMessage += "\n";
		free(message);
	}
	ctx->InfoQueue->ClearStoredMessages();

	return num > 0;
}


// unfortunately no great ways to get this pointer to the callback without a global
main::State* WndProc_State = nullptr;


// Main code
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)pCmdLine;
	(void)nCmdShow;

	// Establish the full path to the current exe and its directory
	std::string ExePath;
	std::string ExeDirectoryPath;
	{
		const char* exeName = "renderland.exe";
		char PathBuffer[2048];
		fileio::GetModuleFileName(nullptr, PathBuffer, ARRAYSIZE(PathBuffer));
		ExePath = PathBuffer;

		size_t exePos = ExePath.rfind(exeName);
		Assert(exePos != std::string::npos, "Could not find exe path in string %s",
			ExePath.c_str());
		ExeDirectoryPath = ExePath.substr(0, exePos);
	}

	std::string ConfigPath = ExeDirectoryPath + "rl.cfg";

	main::State State;
	main::Initialize(&State, ConfigPath.c_str());
	State.RetrieveDisplayTextureID = RetrieveDisplayTextureID;
	State.CheckD3DValidation = CheckD3DValidation;
	State.OnBeforeUnload = OnBeforeUnload;

	WndProc_State = &State;

	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, 
		NULL, NULL, NULL, NULL, "renderland", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "RenderLand (DX12)", WS_OVERLAPPEDWINDOW, 
		State.Cfg.WindowPosX, State.Cfg.WindowPosY, State.Cfg.WindowWidth, 
		State.Cfg.WindowHeight, NULL, NULL, wc.hInstance, NULL);

	// Show the window
	::ShowWindow(hwnd, State.Cfg.Maximized ? SW_MAXIMIZE : SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	gfx::Context Gfx = {};
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = gfx::Context::NUM_BACK_BUFFERS;
		sd.Width = 0;
		sd.Height = 0;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.Stereo = FALSE;

		HRESULT hr;
		// Debug interface must be enabled before device creation
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&Gfx.Debug));
		CheckHresult(hr, "debug interface");
		Gfx.Debug->EnableDebugLayer();
		ID3D12Debug1* d1;
		hr = Gfx.Debug->QueryInterface(IID_PPV_ARGS(&d1));
		CheckHresult(hr, "debug1");
		d1->SetEnableGPUBasedValidation(true);
		d1->Release();

		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
		hr = D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&Gfx.Device));
		CheckHresult(hr, "device");

		hr = Gfx.Device->QueryInterface(IID_PPV_ARGS(&Gfx.InfoQueue));
		CheckHresult(hr, "debug info queue");
		// TODO: decide what to do about these warnings
		{
			D3D12_MESSAGE_ID hide [] =
			{
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
			};
			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			Gfx.InfoQueue->AddStorageFilterEntries(&filter);
		}
		if (IsDebuggerPresent())
		{
			Gfx.InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			Gfx.InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			// Gfx.InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		}

		{
			gfx::CreateDescriptorHeap(&Gfx, &Gfx.RtvHeap, L"RtvHeap",
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				gfx::Context::MAX_RTV_DESCS, gfx::Context::NUM_RESERVED_RTV_SLOTS);

			for (u32 i = 0; i < gfx::Context::NUM_BACK_BUFFERS; i++)
			{
				Gfx.BackBufferDescriptor[i] = gfx::GetCPUDescriptor(&Gfx.RtvHeap, i);
			}
		}

		gfx::CreateDescriptorHeap(&Gfx, &Gfx.DsvHeap, L"DsvHeap",
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			gfx::Context::MAX_DSV_DESCS, gfx::Context::NUM_RESERVED_DSV_SLOTS);

		gfx::CreateDescriptorHeap(&Gfx, &Gfx.CbvSrvUavCreationHeap, L"CbvSrvUavCreationHeap",
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			gfx::Context::MAX_CBV_SRV_UAV_DESCS, gfx::Context::NUM_RESERVED_CBV_SRV_UAV_SLOTS);

		gfx::CreateDescriptorHeap(&Gfx, &Gfx.CbvSrvUavHeap, L"CbvSrvUavHeap",
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			gfx::Context::MAX_SHADER_VIS_DESCS, gfx::Context::NUM_RESERVED_SHADER_VIS_SLOTS);


		gfx::CreateDescriptorHeap(&Gfx, &Gfx.SamplerCreationHeap, L"SamplerCreationHeap",
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 
			512, 0);

		gfx::CreateDescriptorHeap(&Gfx, &Gfx.SamplerHeap, L"SamplerCreationHeap",
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,	D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 
			1024, 0);

		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.NodeMask = 0;
			hr = Gfx.Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&Gfx.CommandQueue));
			CheckHresult(hr, "command queue");
		}

		for (UINT i = 0; i < gfx::Context::NUM_FRAMES_IN_FLIGHT; i++)
		{
			hr = Gfx.Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
				IID_PPV_ARGS(&Gfx.FrameContexts[i].CommandAllocator));
			CheckHresult(hr, "command allocator");
		}

		hr = Gfx.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
			Gfx.FrameContexts[0].CommandAllocator, nullptr, 
			IID_PPV_ARGS(&Gfx.CommandList));
		CheckHresult(hr, "command list");
		Gfx.CommandList->Close();

		hr = Gfx.Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Gfx.Fence));
		CheckHresult(hr, "fence");

		Gfx.FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		Assert(Gfx.FenceEvent, "Failed to create fence event");

		{
			IDXGIFactory4* dxgiFactory = nullptr;
			IDXGISwapChain1* swapChain1 = nullptr;
			hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));
			CheckHresult(hr, "dxgi factory");
			hr = dxgiFactory->CreateSwapChainForHwnd(Gfx.CommandQueue, hwnd, &sd, 
				nullptr, nullptr, &swapChain1);
			CheckHresult(hr, "swap chain");
			hr = swapChain1->QueryInterface(IID_PPV_ARGS(&Gfx.SwapChain));
			CheckHresult(hr, "swap chain");
			swapChain1->Release();
			dxgiFactory->Release();
			Gfx.SwapChain->SetMaximumFrameLatency(gfx::Context::NUM_BACK_BUFFERS);
			Gfx.SwapChainWaitableObject = Gfx.SwapChain->GetFrameLatencyWaitableObject();
		}

		SetupBackBuffer(&Gfx);

		// upload buffer / command list
		{
			D3D12_RESOURCE_DESC bufferDesc;
			bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			bufferDesc.Alignment = 0;
			bufferDesc.Width = gfx::Context::UPLOAD_BUFFER_SIZE;
			bufferDesc.Height = 1;
			bufferDesc.DepthOrArraySize = 1;
			bufferDesc.MipLevels = 1;
			bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
			bufferDesc.SampleDesc.Count = 1;
			bufferDesc.SampleDesc.Quality = 0;
			bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			bufferDesc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		 
			D3D12_HEAP_PROPERTIES uploadHeapProperties;
			uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			uploadHeapProperties.CreationNodeMask = 0;
			uploadHeapProperties.VisibleNodeMask = 0;

			hr = Gfx.Device->CreateCommittedResource(&uploadHeapProperties, 
				D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 
				NULL, IID_PPV_ARGS(&Gfx.UploadBufferResource));
			CheckHresult(hr, "upload buffer");
			Gfx.UploadBufferResource->Map(0, nullptr, &Gfx.UploadBufferMem);

			hr = Gfx.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
				Gfx.FrameContexts[0].CommandAllocator, nullptr, 
				IID_PPV_ARGS(&Gfx.UploadCommandList));
			CheckHresult(hr, "command list");
			Gfx.UploadCommandList->Close();
		}
	}

	State.GfxCtx = &Gfx;

	State.StartupComplete = true;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	Assert(gfx::Context::IMGUI_FONT_RESERVED_SRV_SLOT_INDEX == 0, 
		"If this slot changes, the descriptors passed below need to be updated.");
	ImGui_ImplDX12_Init(Gfx.Device, gfx::Context::NUM_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R8G8B8A8_UNORM, Gfx.CbvSrvUavHeap.Object,
		Gfx.CbvSrvUavHeap.Object->GetCPUDescriptorHandleForHeapStart(),
		Gfx.CbvSrvUavHeap.Object->GetGPUDescriptorHandleForHeapStart());


	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		bool Quit = main::DoUpdate(&State);
		if (Quit)
		{
			::PostQuitMessage(0);
			continue;
		}

		// Rendering
		ImGui::Render();

		// begin frame
		HANDLE waitableObjects[] = { Gfx.SwapChainWaitableObject, NULL };
		DWORD numWaitableObjects = 1;

		gfx::Context::Frame* frameCtx = 
			&Gfx.FrameContexts[Gfx.FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT];
		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue != 0) // means no fence was signaled
		{
			frameCtx->FenceValue = 0;
			Gfx.Fence->SetEventOnCompletion(fenceValue, Gfx.FenceEvent);
			waitableObjects[1] = Gfx.FenceEvent;
			numWaitableObjects = 2;
		}

		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		u32 backBufferIdx = Gfx.SwapChain->GetCurrentBackBufferIndex();
		frameCtx->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = Gfx.BackBufferResource[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
		Gfx.CommandList->Reset(frameCtx->CommandAllocator, NULL);
		Gfx.CommandList->ResourceBarrier(1, &barrier);

		// Transition to RTV usable for clear
		TransitionResource(&Gfx, &State.RlfDisplayTex, D3D12_RESOURCE_STATE_RENDER_TARGET);

		const float clear_color_with_alpha[4] =
		{
			State.ClearColor.x * State.ClearColor.w,
			State.ClearColor.y * State.ClearColor.w, 
			State.ClearColor.z * State.ClearColor.w, 
			State.ClearColor.w
		};
		Gfx.CommandList->ClearRenderTargetView(State.RlfDisplayRtv, 
			clear_color_with_alpha, 0, nullptr);
		Gfx.CommandList->ClearDepthStencilView(State.RlfDepthStencilView,
			D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);


		// Dispatch our shader
		main::DoRender(&State);

		const float clear_0[4] = {0,0,0,0};
		Gfx.CommandList->ClearRenderTargetView(Gfx.BackBufferDescriptor[backBufferIdx], 
			clear_0, 0, nullptr);
		Gfx.CommandList->OMSetRenderTargets(1, &Gfx.BackBufferDescriptor[backBufferIdx], 
			FALSE, NULL);
		ID3D12DescriptorHeap* ShaderDescriptorHeaps[2] = {
			Gfx.CbvSrvUavHeap.Object, Gfx.SamplerHeap.Object
		};
		Gfx.CommandList->SetDescriptorHeaps(2, ShaderDescriptorHeaps);

		// Transition to SRV usable for imgui render
		TransitionResource(&Gfx, &State.RlfDisplayTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Gfx.CommandList);

		// transition the RT back to a presentable surface.
		barrier = {};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = Gfx.BackBufferResource[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
		Gfx.CommandList->ResourceBarrier(1, &barrier);
		Gfx.CommandList->Close();
		// Execute all recorded commands. 
		Gfx.CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&Gfx.CommandList);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		Gfx.SwapChain->Present(1, 0); // present with vsync
		// Gfx.SwapChain->Present(0, 0); // present without vsync

		fenceValue = Gfx.FenceLastSignaledValue + 1;
		Gfx.CommandQueue->Signal(Gfx.Fence, fenceValue);
		Gfx.FenceLastSignaledValue = fenceValue;

		Gfx.FrameContexts[Gfx.FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT].FenceValue = 
			fenceValue;
		++Gfx.FrameIndex;

		main::PostFrame(&State);
	}

	WaitForLastSubmittedFrame(&Gfx);

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	main::Shutdown(&State);

	SafeRelease(State.RlfDisplayTex.Resource);
	SafeRelease(State.RlfDepthStencilTex.Resource);

	CleanupBackBuffer(&Gfx);

	Gfx.SwapChain->SetFullscreenState(false, NULL);
	CloseHandle(Gfx.SwapChainWaitableObject); Gfx.SwapChainWaitableObject = nullptr;
	SafeRelease(Gfx.SwapChain);
	for (u32 i = 0; i < gfx::Context::NUM_FRAMES_IN_FLIGHT; ++i)
		SafeRelease(Gfx.FrameContexts[i].CommandAllocator);
	SafeRelease(Gfx.CommandQueue);
	SafeRelease(Gfx.CommandList);
	SafeRelease(Gfx.UploadCommandList);
	Gfx.UploadBufferResource->Unmap(0, nullptr); Gfx.UploadBufferMem = nullptr;
	SafeRelease(Gfx.UploadBufferResource);
	SafeRelease(Gfx.SamplerHeap.Object);
	SafeRelease(Gfx.SamplerCreationHeap.Object);
	SafeRelease(Gfx.CbvSrvUavHeap.Object);
	SafeRelease(Gfx.CbvSrvUavCreationHeap.Object);
	SafeRelease(Gfx.DsvHeap.Object);
	SafeRelease(Gfx.RtvHeap.Object);
	SafeRelease(Gfx.Fence);
	CloseHandle(Gfx.FenceEvent); Gfx.FenceEvent = nullptr;
	SafeRelease(Gfx.InfoQueue);
	SafeRelease(Gfx.Device);
	SafeRelease(Gfx.Debug);

#ifdef _DEBUG
	// Check for leaked D3D/DXGI objects
	IDXGIDebug1* pDebug = NULL;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

void UpdateWindowStats(HWND hWnd, main::State* s)
{
	if (s->StartupComplete)
	{
		WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
		GetWindowPlacement(hWnd, &wp);
		s->Cfg.Maximized = wp.showCmd == SW_MAXIMIZE;
		s->Cfg.WindowPosX = wp.rcNormalPosition.left;
		s->Cfg.WindowPosY = wp.rcNormalPosition.top;
		s->Cfg.WindowWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		s->Cfg.WindowHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
	}
}

void HandleBackBufferResize(gfx::Context* ctx, u32 w, u32 h)
{
	WaitForLastSubmittedFrame(ctx);
	CleanupBackBuffer(ctx);

	HRESULT hr = ctx->SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
	Assert(hr == S_OK, "Failed to resize swapchain, hr=%x", hr);

	SetupBackBuffer(ctx);
}


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, 
	WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	main::State* s = WndProc_State;
	gfx::Context* ctx = s->GfxCtx;
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (s->StartupComplete && wParam != SIZE_MINIMIZED)
		{
			HandleBackBufferResize(ctx, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));

			UpdateWindowStats(hWnd, s);
		}
		return 0;
	case WM_MOVE:
		UpdateWindowStats(hWnd, s);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_DPICHANGED:
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
		{
			const RECT* suggested_rect = (RECT*)lParam;
			::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, 
				suggested_rect->right - suggested_rect->left, 
				suggested_rect->bottom - suggested_rect->top, 
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


#undef SafeRelease


// Project source
#include "config.cpp"
#include "fileio.cpp"
#include "rlf/rlfparser.cpp"
#include "rlf/ast.cpp"
#include "rlf/shaderparser.cpp"
#include "rlf/d3d12/d3d12_rlfinterpreter.cpp"
#include "rlf/rlfinterpreter.cpp"
#include "gui.cpp"
#include "main.cpp"