
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

#define SafeRelease(ref) do { if (ref) { ref->Release(); ref = nullptr; } } while (0);

const int LAYOUT_VERSION = 1;

gfx::Context Gfx;
gfx::Texture				RlfDisplayTex;
gfx::RenderTargetView		RlfDisplayRtv;
gfx::ShaderResourceView		RlfDisplaySrv;
gfx::UnorderedAccessView	RlfDisplayUav;
gfx::Texture				RlfDepthStencilTex;
gfx::DepthStencilView		RlfDepthStencilView;


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
void LoadRlf();
void UnloadRlf();
void ReportError(const std::string& message);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	Gfx = {};

	gfx::Initialize(&Gfx, hwnd);

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
	ImGui_ImplDX11_Init(Gfx.Device, Gfx.DeviceContext);

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
				gfx::Release(RlfDisplayUav);
				gfx::Release(RlfDisplaySrv);
				gfx::Release(RlfDisplayRtv);
				gfx::Release(RlfDisplayTex);
				gfx::Release(RlfDepthStencilView);
				gfx::Release(RlfDepthStencilTex);

				RlfDisplayTex = gfx::CreateTexture2D(&Gfx, DisplaySize.x, DisplaySize.y,
					DXGI_FORMAT_R8G8B8A8_UNORM, 
					(gfx::BindFlag)(gfx::BindFlag_SRV | gfx::BindFlag_UAV | gfx::BindFlag_RTV));
				RlfDisplaySrv = gfx::CreateShaderResourceView(&Gfx, RlfDisplayTex);
				RlfDisplayUav = gfx::CreateUnorderedAccessView(&Gfx, RlfDisplayTex);
				RlfDisplayRtv = gfx::CreateRenderTargetView(&Gfx, RlfDisplayTex);

				RlfDepthStencilTex = gfx::CreateTexture2D(&Gfx, DisplaySize.x, DisplaySize.y,
					DXGI_FORMAT_D32_FLOAT, gfx::BindFlag_DSV);
				RlfDepthStencilView = gfx::CreateDepthStencilView(&Gfx, RlfDepthStencilTex);
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
					gui::DisplayShaderPasses(CurrentRenderDesc);
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
						else if (tune->Type == rlf::Float2Type)
							ch = ImGui::DragFloat2(tune->Name, (float*)&tune->Value.Float4Val.m, 
								0.01f, tune->Min.FloatVal, tune->Max.FloatVal);
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
		ctx.GfxCtx = &Gfx;
		ctx.MainRtv = RlfDisplayRtv;
		ctx.MainRtUav = RlfDisplayUav;
		ctx.DefaultDepthView = RlfDepthStencilView;
		ctx.EvCtx.DisplaySize = DisplaySize;
		ctx.EvCtx.Time = Time;
		ctx.EvCtx.ChangedThisFrameFlags = changed;

		if (RlfCompileSuccess && (changed & VariesByForTexture) != 0)
		{
			rlf::ErrorState ies = {};
			rlf::HandleTextureParametersChanged(CurrentRenderDesc, 
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
			UnloadRlf();
			LoadRlf();
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
		gfx::ClearRenderTarget(&Gfx, RlfDisplayRtv, clear_color_with_alpha);
		gfx::ClearDepth(&Gfx, RlfDepthStencilView, 1.f);

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

		gfx::ClearBackBufferRtv(&Gfx);
		gfx::BindBackBufferRtv(&Gfx);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		gfx::Present(&Gfx, 1);
		//gfx::Present(&Gfx, 0); // Present without vsync

		LastTime = Time;
		Time = max(0, Time + Speed * io.DeltaTime);

		RlfValidationErrorMessage = "Validation error:\n";
		RlfValidationError = gfx::CheckD3DValidation(&Gfx, RlfValidationErrorMessage);

		PrevDisplaySize = DisplaySize;
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	config::SaveConfig(ConfigPath.c_str(), &Cfg);

	UnloadRlf();

	gfx::Release(RlfDisplayTex);
	gfx::Release(RlfDisplayRtv);
	gfx::Release(RlfDisplaySrv);
	gfx::Release(RlfDisplayUav);
	gfx::Release(RlfDepthStencilTex);
	gfx::Release(RlfDepthStencilView);

	gfx::Release(&Gfx);

	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions


void LoadRlf()
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
	rlf::InitD3D(&Gfx, CurrentRenderDesc, DisplaySize, 
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

void UnloadRlf()
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
	UnloadRlf();
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
		if (Gfx.Device != NULL && wParam != SIZE_MINIMIZED)
		{
			gfx::HandleBackBufferResize(&Gfx, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));

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




// Project source
#include "config.cpp"
#include "fileio.cpp"
#include "rlf/rlfparser.cpp"
#include "rlf/ast.cpp"
#include "rlf/shaderparser.cpp"


// Headers for render backend last so we can verify only the below source files
//	use it. 
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include "rlf/rlfinterpreter.cpp"
#include "d3d11/gfx.cpp"
#include "gui.cpp"