
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
#include "gfx_types.h"
#include "rlf/rlf.h"
#include "rlf/rlfparser.h"
#include "rlf/rlfinterpreter.h"
#include "rlf/shaderparser.h"
#include "gfx.h"
#include "gui.h"
#include "main.h"


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ImGui_Impl_Init(gfx::Context* ctx){ ImGui_ImplDX11_Init(ctx->Device, ctx->DeviceContext); }
void ImGui_Impl_NewFrame() { ImGui_ImplDX11_NewFrame(); }
void ImGui_Impl_RenderDrawData(gfx::Context* ctx, ImDrawData* idd) {
	(void)ctx;
	ImGui_ImplDX11_RenderDrawData(idd);
}
void ImGui_Impl_Shutdown() { ImGui_ImplDX11_Shutdown(); }


ImTextureID RetrieveDisplayTextureID(main::State* s)
{
	gfx::Context* ctx = s->GfxCtx;
	if (gfx::IsNull(&s->RlfDisplayTex) || s->PrevDisplaySize != s->DisplaySize)
	{
		gfx::WaitForLastSubmittedFrame(ctx);
		
		gfx::Release(s->RlfDisplayUav);
		gfx::Release(s->RlfDisplaySrv);
		gfx::Release(s->RlfDisplayRtv);
		gfx::Release(&s->RlfDisplayTex);
		gfx::Release(s->RlfDepthStencilView);
		gfx::Release(&s->RlfDepthStencilTex);

		s->RlfDisplayTex = gfx::CreateTexture2D(ctx, s->DisplaySize.x, s->DisplaySize.y,
			DXGI_FORMAT_R8G8B8A8_UNORM, 
			(gfx::BindFlag)(gfx::BindFlag_SRV | gfx::BindFlag_UAV | gfx::BindFlag_RTV));
		s->RlfDisplaySrv = gfx::CreateShaderResourceView(ctx, &s->RlfDisplayTex);
		s->RlfDisplayUav = gfx::CreateUnorderedAccessView(ctx, &s->RlfDisplayTex);
		s->RlfDisplayRtv = gfx::CreateRenderTargetView(ctx, &s->RlfDisplayTex);

		s->RlfDepthStencilTex = gfx::CreateTexture2D(ctx, s->DisplaySize.x, s->DisplaySize.y,
			DXGI_FORMAT_D32_FLOAT, gfx::BindFlag_DSV);
		s->RlfDepthStencilView = gfx::CreateDepthStencilView(ctx, &s->RlfDepthStencilTex);
	}

	return gfx::GetImTextureID(s->RlfDisplaySrv);
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

	WndProc_State = &State;

	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, 
		NULL, NULL, NULL, NULL, "renderland", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "RenderLand", WS_OVERLAPPEDWINDOW, 
		State.Cfg.WindowPosX, State.Cfg.WindowPosY, State.Cfg.WindowWidth, 
		State.Cfg.WindowHeight, NULL, NULL, wc.hInstance, NULL);

	// Show the window
	::ShowWindow(hwnd, State.Cfg.Maximized ? SW_MAXIMIZE : SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	gfx::Context Gfx = {};
	gfx::Initialize(&Gfx, hwnd);
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
	ImGui_Impl_Init(&Gfx);


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
		ImGui_Impl_NewFrame();
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

		gfx::BeginFrame(&Gfx);

		const float clear_color_with_alpha[4] =
		{
			State.ClearColor.x * State.ClearColor.w,
			State.ClearColor.y * State.ClearColor.w, 
			State.ClearColor.z * State.ClearColor.w, 
			State.ClearColor.w
		};
		gfx::ClearRenderTarget(&Gfx, State.RlfDisplayRtv, clear_color_with_alpha);
		gfx::ClearDepth(&Gfx, State.RlfDepthStencilView, 1.f);

		// Dispatch our shader
		main::DoRender(&State);

		gfx::ClearBackBufferRtv(&Gfx);
		gfx::BindBackBufferRtv(&Gfx);
		ImGui_Impl_RenderDrawData(&Gfx, ImGui::GetDrawData());

		gfx::EndFrame(&Gfx);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		gfx::Present(&Gfx, 1);
		//gfx::Present(&Gfx, 0); // Present without vsync

		main::PostFrame(&State);
	}

	gfx::WaitForLastSubmittedFrame(&Gfx);

	// Cleanup
	ImGui_Impl_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	main::Shutdown(&State);

	gfx::Release(&State.RlfDisplayTex);
	gfx::Release(State.RlfDisplayRtv);
	gfx::Release(State.RlfDisplaySrv);
	gfx::Release(State.RlfDisplayUav);
	gfx::Release(&State.RlfDepthStencilTex);
	gfx::Release(State.RlfDepthStencilView);

	gfx::Release(&Gfx);

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
			gfx::HandleBackBufferResize(ctx, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));

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


// Project source
#include "config.cpp"
#include "fileio.cpp"
#include "rlf/rlfparser.cpp"
#include "rlf/ast.cpp"
#include "rlf/shaderparser.cpp"
#include "d3d11/d3d11_gfx.cpp"
#include "rlf/d3d11/d3d11_rlfinterpreter.cpp"
#include "gui.cpp"
#include "rlf/rlfinterpreter.cpp"
#include "main.cpp"