
// System headers
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string>

// External headers
#include "imgui/imgui.h"

// Imgui example backend
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// Project headers
#include "types.h"
#include "assert.h"
#include "fileio.h"

#define SafeRelease(ref) do { if (ref) { ref->Release(); ref = nullptr; } } while (0);

static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;
static ID3D11ComputeShader*     g_computeShader = nullptr;
static ID3D11UnorderedAccessView*  g_mainRenderTargetUav = NULL;
static ID3D11Buffer*            g_pConstantBuffer = nullptr;

bool ShaderCompileSuccess = false;
std::string ShaderCompileErrorMessage;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void CreateShader();
void CleanupShader();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)pCmdLine;
	(void)nCmdShow;

	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, 
		NULL, NULL, NULL, NULL, "renderland", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, "Renderland",
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

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

	CreateRenderTarget();

	CreateShader();

	struct ConstantBuffer
	{
		float DisplaySizeX, DisplaySizeY;
		float Time;
		float Padding;
	};

	// Create constant buffer
	{
		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = sizeof(ConstantBuffer);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		hr = g_pd3dDevice->CreateBuffer(&desc, NULL, &g_pConstantBuffer);
		Assert(hr == S_OK, "Failed to create CB hr=%x", hr);
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

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
			if (ImGui::Button("Reload shader") || ImGui::IsKeyDown(VK_F5))
			{
				CleanupShader();
				CreateShader();
			}
			if (!ShaderCompileSuccess)
			{
				ImVec4 color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
		        ImGui::PushTextWrapPos(0.f);

				ImGui::TextUnformatted(ShaderCompileErrorMessage.c_str());

				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();
			}

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
				1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, 
			clear_color_with_alpha);

		// Dispatch our shader
		if (ShaderCompileSuccess)
		{
			// D3D11 on PC doesn't like same resource being bound as RT and UAV simultaneously.
			//	Swap to UAV for compute shader. 
			ID3D11RenderTargetView* emptyRT = nullptr;
			g_pd3dDeviceContext->OMSetRenderTargets(1, &emptyRT, nullptr);

			// Update constant buffer
			{
				D3D11_MAPPED_SUBRESOURCE mapped_resource;
				hr = g_pd3dDeviceContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 
					0, &mapped_resource);
				Assert(hr == S_OK, "failed to map CB hr=%x", hr);
				ConstantBuffer* cb = (ConstantBuffer*)mapped_resource.pData;
				cb->DisplaySizeX = io.DisplaySize.x;
				cb->DisplaySizeY = io.DisplaySize.y;
				cb->Time = time;
				g_pd3dDeviceContext->Unmap(g_pConstantBuffer, 0);
			}

			g_pd3dDeviceContext->CSSetShader(g_computeShader, nullptr, 0);
			g_pd3dDeviceContext->CSSetConstantBuffers(0, 1, &g_pConstantBuffer);
			UINT initialCount = (UINT)-1;
			g_pd3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &g_mainRenderTargetUav, 
				&initialCount);
			u32 groupWidth = (u32)((io.DisplaySize.x - 1) / 8) + 1;
			u32 groupHeight = (u32)((io.DisplaySize.y - 1) / 8) + 1;
			g_pd3dDeviceContext->Dispatch(groupWidth,groupHeight,1);

			// D3D11 on PC doesn't like same resource being bound as RT and UAV simultaneously.
			//	Swap back to RT for drawing. 
			ID3D11UnorderedAccessView* emptyUAV = nullptr;
			g_pd3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &emptyUAV, &initialCount);
			g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		}

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync

		time += io.DeltaTime;
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupShader();
	CleanupRenderTarget();
	SafeRelease(g_pSwapChain);
	SafeRelease(g_pd3dDeviceContext);
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
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
	g_pd3dDevice->CreateUnorderedAccessView(pBackBuffer, NULL, &g_mainRenderTargetUav);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	SafeRelease(g_mainRenderTargetView);
	SafeRelease(g_mainRenderTargetUav);
}

void CreateShader()
{
	char* filename = "E:\\dev\\renderland\\source\\shader.hlsl";

	HANDLE file = fileio::OpenFileOptional(filename, GENERIC_READ);

	if (file == INVALID_HANDLE_VALUE) // file not found
	{
		ShaderCompileSuccess = false;
		ShaderCompileErrorMessage = std::string("Couldn't find file: ") + 
			filename;
		return;
	}

	u32 shaderSize = fileio::GetFileSize(file);

	char* buffer = (char*)malloc(shaderSize);	
	Assert(buffer != nullptr, "failed to alloc");

	fileio::ReadFile(file, buffer, shaderSize);

	CloseHandle(file);
	
	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	HRESULT hr = D3DCompile(buffer, shaderSize, filename, NULL, NULL, "CSMain", "cs_5_0",
		0, 0, &shaderBlob, &errorBlob);
	Assert(hr == S_OK || hr == E_FAIL, "failed to compile shader hr=%x", hr);

	ShaderCompileSuccess = (hr == S_OK);

	if (ShaderCompileSuccess)
	{
		hr = g_pd3dDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &g_computeShader);
		Assert(hr == S_OK, "Failed to create shader hr=%x", hr);

		shaderBlob->Release();
	}
	else
	{
		Assert(errorBlob != nullptr, "no error info given");
		char* errorText = (char*)errorBlob->GetBufferPointer();
		ShaderCompileErrorMessage = errorText;

		SafeRelease(errorBlob);
	}

	free(buffer);
}

void CleanupShader()
{
	SafeRelease(g_computeShader);
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
		}
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
#include "fileio.cpp"
