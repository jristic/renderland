
// System headers
#include <windows.h>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <set>

// External headers
#include "imgui/imgui.h"
#include "DirectXTex/DirectXTex.h"

// Imgui example backend
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// Project headers
#include "types.h"
#include "math.h"
#include "matrix.h"
#include "assert.h"
#include "config.h"
#include "fileio.h"
#include "rlf/rlf.h"
#include "rlf/rlfparser.h"
#include "rlf/rlfinterpreter.h"

#define SafeRelease(ref) do { if (ref) { ref->Release(); ref = nullptr; } } while (0);

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11Debug*             g_d3dDebug = nullptr;
static ID3D11InfoQueue*			g_d3dInfoQueue = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static ID3D11UnorderedAccessView*  g_mainRenderTargetUav = nullptr;
static ID3D11Texture2D* 		g_mainDepthStencilTex = nullptr;
static ID3D11DepthStencilView*  g_mainDepthStencilView = nullptr;

char* RlfFile = nullptr;
u32 RlfFileSize = 0;
bool RlfCompileSuccess = false;
std::string RlfCompileErrorMessage;
bool RlfCompileWarning = false;
std::string RlfCompileWarningMessage;
bool RlfValidationError = false;
std::string RlfValidationErrorMessage;
config::Parameters Cfg = { "", false, 0, 0, 1280, 800 };
bool StartupComplete = false;

rlf::RenderDescription* CurrentRenderDesc;

// Forward declarations of helper functions
void CreateRenderTarget();
void CleanupRenderTarget();
void CreateShader();
void CleanupShader();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CheckD3DValidation(std::string& outMessage)
{
	UINT64 num = g_d3dInfoQueue->GetNumStoredMessages();
	for (u32 i = 0 ; i < num ; ++i)
	{
		size_t messageLength;
		HRESULT hr = g_d3dInfoQueue->GetMessage(i, nullptr, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageLength);
		g_d3dInfoQueue->GetMessage(i, message, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		outMessage += message->pDescription;
		free(message);
	}
	g_d3dInfoQueue->ClearStoredMessages();

	return num > 0;
}

std::string RlfFileLocation(const char* filename, const char* location)
{
	u32 line = 1;
	u32 chr = 0;
	const char* lineStart = RlfFile;
	for (const char* b = RlfFile; b < location ; ++b)
	{
		if (*b == '\n')
		{
			lineStart = b+1;
			++line;
			chr = 0;
		}
		else if (*b == '\t')
			chr += 4;
		else
			++chr;
	}

	const char* lineEnd = lineStart;
	while (lineEnd < RlfFile + RlfFileSize && *lineEnd != '\n')
		++lineEnd;

	char buf[512];
	sprintf_s(buf, 512, "%s(%u,%u):\n%.*s \n", filename, line, chr, (u32)(lineEnd-lineStart),
		lineStart);
	std::string str = buf;
	if (chr + 2 < 512)
	{
		for (u32 i = 0 ; i < chr ; ++i)
			buf[i] = ' ';
		buf[chr] = '^';
		buf[chr+1] = '\0';
		str += buf;
	}
	return str;
}

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

	LoadConfig(ConfigPath.c_str(), &Cfg);

	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, 
		NULL, NULL, NULL, NULL, "renderland", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "RenderLand", WS_OVERLAPPEDWINDOW, 
		Cfg.WindowPosX, Cfg.WindowPosY, Cfg.WindowWidth, Cfg.WindowHeight, NULL, 
		NULL, wc.hInstance, NULL);

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
	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
		createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
		&g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	Assert(hr == S_OK, "failed to create device %x", hr);

	BOOL success = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), 
		(void**)&g_d3dInfoQueue);
	Assert(SUCCEEDED(success), "failed to get debug device");
	success = g_d3dInfoQueue->QueryInterface(__uuidof(ID3D11InfoQueue),
		(void**)&g_d3dInfoQueue);
	Assert(SUCCEEDED(success), "failed to get info queue");
	// D3D11_MESSAGE_ID hide[] = {
	   //  D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
	// };

	// D3D11_INFO_QUEUE_FILTER filter = {};
	// filter.DenyList.NumIDs = _countof(hide);
	// filter.DenyList.pIDList = hide;
	// g_d3dInfoQueue->AddStorageFilterEntries(&filter);
	if (IsDebuggerPresent())
	{
		g_d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
		g_d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	}

	CreateRenderTarget();

	CreateShader();

	// Show the window
	::ShowWindow(hwnd, Cfg.Maximized ? SW_MAXIMIZE : SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	StartupComplete = true;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	float time = 0;

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

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Shader");

			ImGui::ColorEdit3("Clear color", (float*)&clear_color);
			ImGui::Text("DisplaySize = %u / %u", (u32)io.DisplaySize.x, (u32)io.DisplaySize.y);
			ImGui::Text("Time = %f", time);
			ImGui::InputText("Rlf path", Cfg.FilePath, IM_ARRAYSIZE(Cfg.FilePath));
			bool reload = ImGui::IsItemDeactivatedAfterEdit();
			reload = ImGui::Button("Reload shader") || reload;
			reload = ImGui::IsKeyDown(VK_F5) || reload;
			if (reload)
			{
				time = 0;
				CleanupShader();
				CreateShader();
			}
			if (!RlfCompileSuccess)
			{
				ImVec4 color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::PushTextWrapPos(0.f);

				ImGui::TextUnformatted(RlfCompileErrorMessage.c_str());

				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
			}
			if (RlfCompileWarning)
			{
				ImVec4 color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::PushTextWrapPos(0.f);

				ImGui::TextUnformatted(RlfCompileWarningMessage.c_str());

				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
			}
			if (RlfValidationError)
			{
				ImVec4 color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::PushTextWrapPos(0.f);

				ImGui::TextUnformatted(RlfValidationErrorMessage.c_str());

				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
			}

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
				1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::Text("Exe directory: %s", ExeDirectoryPath.c_str());

			if (RlfCompileSuccess)
			{
				ImGui::Text("Tuneables:");
				for (rlf::Tuneable* tune : CurrentRenderDesc->Tuneables)
				{
					if (tune->Type == rlf::BoolType)
						ImGui::Checkbox(tune->Name, &tune->Value.BoolVal);
					else if (tune->Type == rlf::FloatType)
						ImGui::DragFloat(tune->Name, &tune->Value.FloatVal, 1.f,
							tune->Min.FloatVal, tune->Max.FloatVal);
					else if (tune->Type == rlf::IntType)
						ImGui::DragInt(tune->Name, &tune->Value.IntVal, 1.f, 
							tune->Min.IntVal, tune->Max.IntVal);
					else if (tune->Type == rlf::UintType)
					{
						i32 max = (tune->Min.UintVal == tune->Max.UintVal && 
							tune->Min.UintVal == 0) ? INT_MAX : tune->Max.UintVal;
						ImGui::DragInt(tune->Name, (i32*)&tune->Value.UintVal,
							1, tune->Min.IntVal, max, "%d", ImGuiSliderFlags_AlwaysClamp);
					}
					else
						Unimplemented();
				}
			}

			ImGui::End();
		}

		// ImGui::ShowDemoWindow(nullptr);

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] =
		{
			clear_color.x * clear_color.w,
			clear_color.y * clear_color.w, 
			clear_color.z * clear_color.w, 
			clear_color.w
		};
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, 
			clear_color_with_alpha);
		g_pd3dDeviceContext->ClearDepthStencilView(g_mainDepthStencilView, 
			D3D11_CLEAR_DEPTH, 1.f, 0);

		// Dispatch our shader
		if (RlfCompileSuccess)
		{
			rlf::ExecuteContext ctx = {};
			ctx.D3dCtx = g_pd3dDeviceContext;
			ctx.MainRtv = g_mainRenderTargetView;
			ctx.MainRtUav = g_mainRenderTargetUav;
			ctx.DefaultDepthView = g_mainDepthStencilView;
			ctx.DisplaySize.x = (u32)io.DisplaySize.x;
			ctx.DisplaySize.y = (u32)io.DisplaySize.y;
			ctx.Time = time;
			rlf::ExecuteErrorState es = {};
			rlf::Execute(&ctx, CurrentRenderDesc, &es);
			if (!es.ExecuteSuccess)
			{
				RlfCompileSuccess = false;
				RlfCompileErrorMessage = "RLF execution error: \n" + es.Info.Message;
				if (es.Info.Location)
					RlfCompileErrorMessage += "\n" + 
						RlfFileLocation(Cfg.FilePath, es.Info.Location);
			}
		}

		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync

		time += io.DeltaTime;

		RlfValidationErrorMessage = "Validation error:\n";
		RlfValidationError = CheckD3DValidation(RlfValidationErrorMessage);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	config::SaveConfig(ConfigPath.c_str(), &Cfg);

	CleanupShader();
	CleanupRenderTarget();

	SafeRelease(g_pSwapChain);
	SafeRelease(g_pd3dDeviceContext);
	SafeRelease(g_d3dInfoQueue);
	SafeRelease(g_d3dDebug);
	SafeRelease(g_pd3dDevice);

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	// TODO: Should we not be being using an SRGB conversion on the backbuffer? 
	//	From my understanding we should, but it looks visually wrong to me. 
	// D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	// rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &rtvDesc, &g_mainRenderTargetView);
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	g_pd3dDevice->CreateUnorderedAccessView(pBackBuffer, nullptr, &g_mainRenderTargetUav);
	pBackBuffer->Release();

	D3D11_TEXTURE2D_DESC rtDesc = {};
	pBackBuffer->GetDesc(&rtDesc);

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = rtDesc.Width;
	desc.Height = rtDesc.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_mainDepthStencilTex);
	Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

	hr = g_pd3dDevice->CreateDepthStencilView(g_mainDepthStencilTex, nullptr, 
		&g_mainDepthStencilView);
	Assert(hr == S_OK, "failed to create depthstencil view, hr=%x", hr);
}

void CleanupRenderTarget()
{
	SafeRelease(g_mainRenderTargetView);
	SafeRelease(g_mainRenderTargetUav);
	SafeRelease(g_mainDepthStencilView);
	SafeRelease(g_mainDepthStencilTex);
}

void CreateShader()
{
	RlfCompileSuccess = true;
	RlfCompileWarning = false;
	RlfValidationError = false;

	char* filename = Cfg.FilePath;

	std::string filePath(filename);
	std::string dirPath;
	size_t pos = filePath.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		dirPath = filePath.substr(0, pos+1);
	}

	HANDLE rlf = fileio::OpenFileOptional(filename, GENERIC_READ);

	if (rlf == INVALID_HANDLE_VALUE) // file not found
	{
		RlfCompileSuccess = false;
		RlfCompileErrorMessage = std::string("Couldn't find ") + filename;
		return;
	}

	RlfFileSize = fileio::GetFileSize(rlf);

	Assert(!RlfFile, "Leak");
	RlfFile = (char*)malloc(RlfFileSize);	
	Assert(RlfFile != nullptr, "failed to alloc");

	fileio::ReadFile(rlf, RlfFile, RlfFileSize);

	CloseHandle(rlf);

	Assert(CurrentRenderDesc == nullptr, "leaking data");
	rlf::ParseErrorState es = {};
	CurrentRenderDesc = rlf::ParseBuffer(RlfFile, RlfFileSize, dirPath.c_str(), &es);

	if (es.ParseSuccess == false)
	{
		Assert(CurrentRenderDesc == nullptr, "leaking data");
		RlfCompileSuccess = false;
		RlfCompileErrorMessage = std::string("Failed to parse RLF:\n") + es.Info.Message +
			"\n" + RlfFileLocation(filename, es.Info.Location);
		free(RlfFile);
		RlfFile = nullptr;
		return;
	}

	rlf::InitErrorState ies = {};
	rlf::InitD3D(g_pd3dDevice, CurrentRenderDesc, dirPath.c_str(), &ies);

	if (ies.InitSuccess == false)
	{
		CleanupShader();
		RlfCompileSuccess = false;
		RlfCompileErrorMessage = std::string("Failed to create RLF scene:\n") +
			ies.ErrorMessage;
		return;
	}

	RlfCompileWarning = ies.InitWarning;
	RlfCompileWarningMessage = ies.ErrorMessage;
}

void CleanupShader()
{
	if (CurrentRenderDesc)
	{
		rlf::ReleaseD3D(CurrentRenderDesc);
		rlf::ReleaseData(CurrentRenderDesc);
		CurrentRenderDesc = nullptr;
		free(RlfFile);
		RlfFile = nullptr;
	}
}

void UpdateWindowStats(HWND hWnd)
{
	if (StartupComplete)
	{
		WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
		GetWindowPlacement(hWnd, &wp);
		Cfg.Maximized = wp.showCmd == SW_MAXIMIZE;
		Cfg.WindowPosX = wp.rcNormalPosition.left;
		Cfg.WindowPosY = wp.rcNormalPosition.top;
		Cfg.WindowWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		Cfg.WindowHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
	}
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, 
	WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), 
				DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();

			UpdateWindowStats(hWnd);
		}
		return 0;
	case WM_MOVE:
		UpdateWindowStats(hWnd);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

// External source
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
// #include "imgui/imgui_demo.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"

// Imgui example backend
#include "imgui_impl_win32.cpp"
#include "imgui_impl_dx11.cpp"

// Project source
#include "config.cpp"
#include "fileio.cpp"
#include "rlf/rlfparser.cpp"
#include "rlf/rlfinterpreter.cpp"
#include "rlf/ast.cpp"
