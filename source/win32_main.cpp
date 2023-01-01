
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
#include <unordered_set>
#include <set>

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

// External headers
#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "DirectXTex/DirectXTex.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

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

const int LAYOUT_VERSION = 1;

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11Debug*             g_d3dDebug = nullptr;
static ID3D11InfoQueue*			g_d3dInfoQueue = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

static ID3D11Texture2D* 		RlfDisplayTex = nullptr;
static ID3D11RenderTargetView*  RlfDisplayRtv = nullptr;
static ID3D11ShaderResourceView*  RlfDisplaySrv = nullptr;
static ID3D11UnorderedAccessView*  RlfDisplayUav = nullptr;
static ID3D11Texture2D* 		RlfDepthStencilTex = nullptr;
static ID3D11DepthStencilView*  RlfDepthStencilView = nullptr;

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

uint2 DisplaySize;
uint2 PrevDisplaySize;

rlf::RenderDescription* CurrentRenderDesc;

// Forward declarations of helper functions
void CreateRenderTarget();
void CleanupRenderTarget();
void CreateShader();
void CleanupShader();
void ReportError(const std::string& message);
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
	Assert(filename, "File must be provided.");
	if (!location)
		return "";
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

	// Show the window
	::ShowWindow(hwnd, Cfg.Maximized ? SW_MAXIMIZE : SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

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
		(void**)&g_d3dDebug);
	Assert(SUCCEEDED(success), "failed to get debug device");
	success = g_d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue),
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

	StartupComplete = true;

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
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	float Time = 0;
	float LastTime = -1;
	float Speed = 1;
	bool FirstLoad = true;
	// bool showDemoWindow = true;
	bool showPlaybackWindow = true;
	bool showParametersWindow = true;
	bool showEventsWindow = true;

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

		bool Reload = FirstLoad || ImGui::IsKeyReleased(ImGuiKey_F5);
		bool Quit = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_Q);
		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyReleased(ImGuiKey_O))
			ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
				"Choose File", ".rlf", ".");

		bool TuneablesChanged = false;
		bool ResetLayout = false;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Reload", "F5"))
					Reload = true;
				if (ImGui::MenuItem("Open", "Ctrl+O"))
					ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
						"Choose File", ".rlf", ".");
				ImGui::Separator();
				if (ImGui::MenuItem("Quit", "Ctrl+Q"))
					Quit = true;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Windows"))
			{
				ImGui::MenuItem("Event Viewer", "", &showEventsWindow);
				ImGui::MenuItem("Playback", "", &showPlaybackWindow);
				ImGui::MenuItem("Parameters", "", &showParametersWindow);
				ImGui::Separator();
				if (ImGui::MenuItem("Reset layout"))
					ResetLayout = true;
				ImGui::EndMenu();
			}
			ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
			ImGui::Text("Loaded: %s", Cfg.FilePath);
			ImGui::EndMainMenuBar();
		}

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_PassthruCentralNode);

		if (Cfg.LayoutVersionApplied != LAYOUT_VERSION || ResetLayout)
		{
			// Clear out existing layout (includes children)
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

			ImGuiID dock_main_id = dockspace_id;
			ImGuiID dockspace_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, 
				ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);
			ImGuiID dockspace_left_id = ImGui::DockBuilderSplitNode(dock_main_id, 
				ImGuiDir_Left, 0.15f, nullptr, &dock_main_id);
			ImGuiID dockspace_right_id = ImGui::DockBuilderSplitNode(dock_main_id,
				ImGuiDir_Right, 0.30f, nullptr, &dock_main_id);
			ImGuiID dockspace_right_top_id = ImGui::DockBuilderSplitNode(dockspace_right_id,
				ImGuiDir_Up, 0.10f, nullptr, &dockspace_right_id);

			ImGui::DockBuilderDockWindow("Display", dock_main_id);
			ImGui::DockBuilderDockWindow("Compile Output", dockspace_bottom_id);
			ImGui::DockBuilderDockWindow("Event Viewer", dockspace_left_id);
			ImGui::DockBuilderDockWindow("Playback", dockspace_right_top_id);
			ImGui::DockBuilderDockWindow("Parameters", dockspace_right_id);
			ImGui::DockBuilderFinish(dockspace_id);

			Cfg.LayoutVersionApplied = LAYOUT_VERSION;
		}

		if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				memcpy_s(Cfg.FilePath, sizeof(Cfg.FilePath), filePathName.c_str(), 
					filePathName.length());
				Cfg.FilePath[filePathName.length()] = '\0';
				Reload = true;
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// if (showDemoWindow) ImGui::ShowDemoWindow(&showDemoWindow);
		if (Quit)
		{
			::PostQuitMessage(0);
			continue;
		}

		ImGui::Begin("Display", nullptr, ImGuiWindowFlags_NoCollapse);
		{
			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();
			DisplaySize.x = (u32)max(1, vMax.x - vMin.x);
			DisplaySize.y = (u32)max(1, vMax.y - vMin.y);

			if (RlfDisplayTex == nullptr || PrevDisplaySize != DisplaySize)
			{
				SafeRelease(RlfDisplayUav);
				SafeRelease(RlfDisplaySrv);
				SafeRelease(RlfDisplayRtv);
				SafeRelease(RlfDisplayTex);
				SafeRelease(RlfDepthStencilView);
				SafeRelease(RlfDepthStencilTex);

				D3D11_TEXTURE2D_DESC desc = {};
				desc.Width = DisplaySize.x;
				desc.Height = DisplaySize.y;
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
				hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &RlfDisplayTex);
				Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

				g_pd3dDevice->CreateRenderTargetView(RlfDisplayTex, nullptr, &RlfDisplayRtv);
				g_pd3dDevice->CreateUnorderedAccessView(RlfDisplayTex, nullptr, &RlfDisplayUav);
				g_pd3dDevice->CreateShaderResourceView(RlfDisplayTex, nullptr, &RlfDisplaySrv);

				desc = {};
				desc.Width = DisplaySize.x;
				desc.Height = DisplaySize.y;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_D32_FLOAT;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;

				hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &RlfDepthStencilTex);
				Assert(hr == S_OK, "failed to create texture, hr=%x", hr);
				hr = g_pd3dDevice->CreateDepthStencilView(RlfDepthStencilTex, nullptr, 
					&RlfDepthStencilView);
				Assert(hr == S_OK, "failed to create depthstencil view, hr=%x", hr);
			}
			
			ImGui::Image(RlfDisplaySrv, ImVec2(vMax.x-vMin.x, vMax.y-vMin.y));
		}
		ImGui::End();

		if (ImGui::Begin("Compile Output"))
		{
			static float f = 0.0f;

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
		}
		ImGui::End();

		if (showEventsWindow)
		{
			if (ImGui::Begin("Event Viewer", &showEventsWindow))
			{
				if (RlfCompileSuccess)
				{
					std::vector<rlf::Pass> passes = CurrentRenderDesc->Passes;
					for (int i = 0 ; i < passes.size() ; ++i)
					{
						rlf::Pass& p = passes[i];
						static int selected_index = -1;
						if (ImGui::Selectable(p.Name ? p.Name : "anon", selected_index == i))
							selected_index = i;
					}
				}
			}
			ImGui::End();
		}

		if (showPlaybackWindow)
		{
			if (ImGui::Begin("Playback", &showPlaybackWindow))
			{
				if (ImGui::Button("<<"))
					Speed = -2;
				ImGui::SameLine();
				if (ImGui::Button("<"))
					Speed = -1;
				ImGui::SameLine();
				if (ImGui::Button("||"))
					Speed = 0;
				ImGui::SameLine();
				if (ImGui::Button(">"))
					Speed = 1;
				ImGui::SameLine();
				if (ImGui::Button(">>"))
					Speed = 2;
				ImGui::Text("Time = %f", Time);
			}
			ImGui::End();
		}

		if (showParametersWindow)
		{
			if (ImGui::Begin("Parameters", &showParametersWindow))
			{
				ImGui::ColorEdit3("Clear color", (float*)&clear_color);
				ImGui::Text("DisplaySize = %u / %u", DisplaySize.x, DisplaySize.y);
				if (RlfCompileSuccess)
				{
					ImGui::Text("Tuneables:");
					for (rlf::Tuneable* tune : CurrentRenderDesc->Tuneables)
					{
						bool ch = false;
						if (tune->Type == rlf::BoolType)
							ch = ImGui::Checkbox(tune->Name, &tune->Value.BoolVal);
						else if (tune->Type == rlf::FloatType)
							ch = ImGui::DragFloat(tune->Name, &tune->Value.FloatVal, 0.01f,
								tune->Min.FloatVal, tune->Max.FloatVal);
						else if (tune->Type == rlf::Float3Type)
							ch = ImGui::DragFloat3(tune->Name, (float*)&tune->Value.Float4Val.m, 
								0.01f, tune->Min.FloatVal, tune->Max.FloatVal);
						else if (tune->Type == rlf::IntType)
							ch = ImGui::DragInt(tune->Name, &tune->Value.IntVal, 1.f, 
								tune->Min.IntVal, tune->Max.IntVal);
						else if (tune->Type == rlf::UintType)
						{
							i32 max = (tune->Min.UintVal == tune->Max.UintVal && 
								tune->Min.UintVal == 0) ? INT_MAX : tune->Max.UintVal;
							ch = ImGui::DragInt(tune->Name, (i32*)&tune->Value.UintVal,
								1, tune->Min.IntVal, max, "%d", ImGuiSliderFlags_AlwaysClamp);
						}
						else
							Unimplemented();
						TuneablesChanged |= ch;
					}
				}
			}
			ImGui::End();
		}

		u32 changed = 0;
		changed |= (DisplaySize != PrevDisplaySize) ? rlf::ast::VariesBy_DisplaySize : 0;
		changed |= TuneablesChanged ? rlf::ast::VariesBy_Tuneable : 0;
		changed |= LastTime != Time ? rlf::ast::VariesBy_Time : 0;

		u32 VariesByForTexture = rlf::ast::VariesBy_Tuneable | rlf::ast::VariesBy_DisplaySize;

		rlf::ExecuteContext ctx = {};
		ctx.D3dCtx = g_pd3dDeviceContext;
		ctx.MainRtv = RlfDisplayRtv;
		ctx.MainRtUav = RlfDisplayUav;
		ctx.DefaultDepthView = RlfDepthStencilView;
		ctx.EvCtx.DisplaySize = DisplaySize;
		ctx.EvCtx.Time = Time;
		ctx.EvCtx.ChangedThisFrameFlags = changed;

		if (RlfCompileSuccess && (changed & VariesByForTexture) != 0)
		{
			rlf::ErrorState ies = {};
			rlf::HandleTextureParametersChanged(g_pd3dDevice, CurrentRenderDesc, 
				&ctx, &ies);
			if (!ies.Success)
			{
				ReportError( std::string("Error resizing textures:\n") + ies.Info.Message + 
					"\n" + RlfFileLocation(Cfg.FilePath, ies.Info.Location));
			}
		}

		if (Reload)
		{
			FirstLoad = false;
			Time = 0;
			CleanupShader();
			CreateShader();
		}

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] =
		{
			clear_color.x * clear_color.w,
			clear_color.y * clear_color.w, 
			clear_color.z * clear_color.w, 
			clear_color.w
		};
		g_pd3dDeviceContext->ClearRenderTargetView(RlfDisplayRtv, 
			clear_color_with_alpha);
		g_pd3dDeviceContext->ClearDepthStencilView(RlfDepthStencilView, 
			D3D11_CLEAR_DEPTH, 1.f, 0);

		// Dispatch our shader
		if (RlfCompileSuccess)
		{
			rlf::ErrorState es = {};
			rlf::Execute(&ctx, CurrentRenderDesc, &es);
			if (!es.Success)
			{
				ReportError( "RLF execution error: \n" + es.Info.Message +
					"\n" + RlfFileLocation(Cfg.FilePath, es.Info.Location));
			}
		}

		const float clear_0[4] = {0,0,0,0};
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, 
			clear_0);
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync

		LastTime = Time;
		Time = max(0, Time + Speed * io.DeltaTime);

		RlfValidationErrorMessage = "Validation error:\n";
		RlfValidationError = CheckD3DValidation(RlfValidationErrorMessage);

		PrevDisplaySize = DisplaySize;
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	config::SaveConfig(ConfigPath.c_str(), &Cfg);

	CleanupShader();
	CleanupRenderTarget();

	SafeRelease(RlfDisplayTex);
	SafeRelease(RlfDisplayRtv);
	SafeRelease(RlfDisplaySrv);
	SafeRelease(RlfDisplayUav);
	SafeRelease(RlfDepthStencilTex);
	SafeRelease(RlfDepthStencilView);

	SafeRelease(g_pSwapChain);
	SafeRelease(g_pd3dDeviceContext);
	SafeRelease(g_d3dInfoQueue);
	SafeRelease(g_d3dDebug);
	SafeRelease(g_pd3dDevice);

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
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	SafeRelease(g_mainRenderTargetView);
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
		ReportError( std::string("Couldn't find ") + filename);
		return;
	}

	RlfFileSize = fileio::GetFileSize(rlf);

	Assert(!RlfFile, "Leak");
	RlfFile = (char*)malloc(RlfFileSize);	
	Assert(RlfFile != nullptr, "failed to alloc");

	fileio::ReadFile(rlf, RlfFile, RlfFileSize);

	CloseHandle(rlf);

	Assert(CurrentRenderDesc == nullptr, "leaking data");
	rlf::ErrorState es = {};
	CurrentRenderDesc = rlf::ParseBuffer(RlfFile, RlfFileSize, dirPath.c_str(), &es);

	if (es.Success == false)
	{
		ReportError( std::string("Failed to parse RLF:\n") + es.Info.Message +
			"\n" + RlfFileLocation(filename, es.Info.Location));
		return;
	}

	es = {};
	rlf::InitD3D(g_pd3dDevice, g_d3dInfoQueue, CurrentRenderDesc, DisplaySize, 
		dirPath.c_str(), &es);

	if (es.Success == false)
	{
		ReportError( std::string("Failed to create RLF scene:\n") +
			es.Info.Message + "\n" + RlfFileLocation(filename, es.Info.Location));
		return;
	}

	RlfCompileWarning = es.Warning;
	RlfCompileWarningMessage = es.Info.Message;
}

void CleanupShader()
{
	if (CurrentRenderDesc)
	{
		rlf::ReleaseD3D(CurrentRenderDesc);
		rlf::ReleaseData(CurrentRenderDesc);
		CurrentRenderDesc = nullptr;
	}
	if (RlfFile)
	{
		free(RlfFile);
		RlfFile = nullptr;
	}
}

void ReportError(const std::string& message)
{
	RlfCompileSuccess = false;
	RlfCompileErrorMessage = message;
	CleanupShader();
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

// External source
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
// #include "imgui/imgui_demo.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#pragma warning( push )
#pragma warning( disable : 4100 )
#include "ImGuiFileDialog/ImGuiFileDialog.cpp"
#pragma warning( pop )

// Imgui example backend
#include "imgui_impl_win32.cpp"
#include "imgui_impl_dx11.cpp"

// Project source
#include "config.cpp"
#include "fileio.cpp"
#include "rlf/rlfparser.cpp"
#include "rlf/rlfinterpreter.cpp"
#include "rlf/ast.cpp"
