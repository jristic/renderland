
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
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#if defined(_DEBUG)
	#include <dxgidebug.h>
#endif

// Project headers
#include "types.h"
#include "math.h"
#include "matrix.h"
#include "assert.h"
#include "config.h"
#include "fileio.h"
#include "d3d11/gfx.h"
#include "rlf/rlf.h"
#include "rlf/rlfparser.h"
#include "rlf/rlfinterpreter.h"
#include "rlf/shaderparser.h"
#include "gui.h"
#include "main.h"


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define SafeRelease(ref) do { if (ref) { (ref)->Release(); (ref) = nullptr; } } while (0);

ImTextureID RetrieveDisplayTextureID(main::State* s)
{
	gfx::Context* ctx = s->GfxCtx;
	if (s->RlfDisplayTex == nullptr || s->PrevDisplaySize != s->DisplaySize)
	{
		SafeRelease(s->RlfDisplayUav);
		SafeRelease(s->RlfDisplaySrv);
		SafeRelease(s->RlfDisplayRtv);
		SafeRelease(s->RlfDisplayTex);

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = s->DisplaySize.x;
		desc.Height = s->DisplaySize.y;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS |
			D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		HRESULT hr = ctx->Device->CreateTexture2D(&desc, nullptr, &s->RlfDisplayTex);
		Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

		ctx->Device->CreateRenderTargetView(s->RlfDisplayTex, nullptr, &s->RlfDisplayRtv);
		ctx->Device->CreateUnorderedAccessView(s->RlfDisplayTex, nullptr, &s->RlfDisplayUav);
		ctx->Device->CreateShaderResourceView(s->RlfDisplayTex, nullptr, &s->RlfDisplaySrv);
	}
	if (s->RlfCompileSuccess)
	{
		Assert(s->CurrentRenderDesc->OutputViews.Count > 0, "No texture to display.");
		return s->CurrentRenderDesc->OutputViews[0];
	}
	else
		return s->RlfDisplaySrv;
}

bool CheckD3DValidation(gfx::Context* ctx, std::string& outMessage)
{
	UINT64 num = ctx->InfoQueue->GetNumStoredMessages();
	for (u32 i = 0 ; i < num ; ++i)
	{
		size_t messageLength;
		HRESULT hr = ctx->InfoQueue->GetMessage(i, nullptr, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageLength);
		ctx->InfoQueue->GetMessage(i, message, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		outMessage += message->pDescription;
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

	WndProc_State = &State;

	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, 
		NULL, NULL, NULL, NULL, "renderland", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "RenderLand (DX11)", WS_OVERLAPPEDWINDOW, 
		State.Cfg.WindowPosX, State.Cfg.WindowPosY, State.Cfg.WindowWidth, 
		State.Cfg.WindowHeight, NULL, NULL, wc.hInstance, NULL);

	// Show the window
	::ShowWindow(hwnd, State.Cfg.Maximized ? SW_MAXIMIZE : SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	gfx::Context Gfx = {};
	{
		// Initialize Direct3D
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		UINT createDeviceFlags = 0;
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_1 };
		HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
			createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &Gfx.SwapChain, 
			&Gfx.Device, &featureLevel, &Gfx.DeviceContext);
		Assert(hr == S_OK, "failed to create device %x", hr);

		BOOL success = Gfx.Device->QueryInterface(__uuidof(ID3D11Debug), 
			(void**)&Gfx.Debug);
		Assert(SUCCEEDED(success), "failed to get debug device");
		success = Gfx.Debug->QueryInterface(__uuidof(ID3D11InfoQueue),
			(void**)&Gfx.InfoQueue);
		Assert(SUCCEEDED(success), "failed to get info queue");
		// D3D11_MESSAGE_ID hide[] = {
		   //  D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
		// };

		// D3D11_INFO_QUEUE_FILTER filter = {};
		// filter.DenyList.NumIDs = _countof(hide);
		// filter.DenyList.pIDList = hide;
		// InfoQueue->AddStorageFilterEntries(&filter);
		if (IsDebuggerPresent())
		{
			Gfx.InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			Gfx.InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
		}

		ID3D11Texture2D* pBackBuffer;
		Gfx.SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		// TODO: Should we not be using an SRGB conversion on the backbuffer? 
		//	From my understanding we should, but it looks visually wrong to me. 
		// D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		// rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		// rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		// Device->CreateRenderTargetView(pBackBuffer, &rtvDesc, &Gfx.BackBufferRtv);
		Gfx.Device->CreateRenderTargetView(pBackBuffer, nullptr, &Gfx.BackBufferRtv);
		pBackBuffer->Release();
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
	ImGui_ImplDX11_Init(Gfx.Device, Gfx.DeviceContext);

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
		ImGui_ImplDX11_NewFrame();
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

		const float clear_color_with_alpha[4] =
		{
			State.ClearColor.x * State.ClearColor.w,
			State.ClearColor.y * State.ClearColor.w, 
			State.ClearColor.z * State.ClearColor.w, 
			State.ClearColor.w
		};
		Gfx.DeviceContext->ClearRenderTargetView(State.RlfDisplayRtv, 
			clear_color_with_alpha);

		// Dispatch our shader
		main::DoRender(&State);

		const float clear_0[4] = {0,0,0,0};
		Gfx.DeviceContext->ClearRenderTargetView(Gfx.BackBufferRtv, clear_0);
		Gfx.DeviceContext->OMSetRenderTargets(1, &Gfx.BackBufferRtv, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		Gfx.SwapChain->Present(1, 0); // Present with vsync
		// Gfx.SwapChain->Present(0, 0); // Present without vsync

		main::PostFrame(&State);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	main::Shutdown(&State);

	SafeRelease(State.RlfDisplayUav);
	SafeRelease(State.RlfDisplaySrv);
	SafeRelease(State.RlfDisplayRtv);
	SafeRelease(State.RlfDisplayTex);

	SafeRelease(Gfx.BackBufferRtv);
	SafeRelease(Gfx.SwapChain);
	SafeRelease(Gfx.DeviceContext);
	SafeRelease(Gfx.InfoQueue);
	SafeRelease(Gfx.Debug);
	SafeRelease(Gfx.Device);

#if defined(_DEBUG)
	// Check for leaked D3D/DXGI objects
	typedef HRESULT (WINAPI * LPDXGIGETDEBUGINTERFACE)(REFIID, void ** );
	HMODULE dxgiDllHandle = LoadLibraryEx( "dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );
	if ( dxgiDllHandle )
	{
		LPDXGIGETDEBUGINTERFACE dxgiGetDebugInterface = reinterpret_cast<LPDXGIGETDEBUGINTERFACE>(
			reinterpret_cast<void*>( GetProcAddress(dxgiDllHandle, "DXGIGetDebugInterface") ));

		IDXGIDebug* dxgiDebug;
		if ( SUCCEEDED( dxgiGetDebugInterface( IID_PPV_ARGS( &dxgiDebug ) ) ) )
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			dxgiDebug->Release();
		}
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
	SafeRelease(ctx->BackBufferRtv);

	ctx->SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* pBackBuffer;
	ctx->SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	// TODO: Should we not be using an SRGB conversion on the backbuffer? 
	//	From my understanding we should, but it looks visually wrong to me. 
	// D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	// rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// Device->CreateRenderTargetView(pBackBuffer, &rtvDesc, &ctx->BackBufferRtv);
	ctx->Device->CreateRenderTargetView(pBackBuffer, nullptr, &ctx->BackBufferRtv);
	pBackBuffer->Release();
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
#include "rlf/d3d11/d3d11_rlfinterpreter.cpp"
#include "rlf/rlfinterpreter.cpp"
#include "rlf/alloc.cpp"
#include "gui.cpp"
#include "main.cpp"