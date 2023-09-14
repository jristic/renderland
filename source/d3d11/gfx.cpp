
namespace gfx
{


#define SafeRelease(ref) do { if (ref) { ref->Release(); ref = nullptr; } } while (0);


void Initialize(Context* ctx, HWND hwnd)
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
	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
		createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &ctx->g_pSwapChain, 
		&ctx->g_pd3dDevice, &featureLevel, &ctx->g_pd3dDeviceContext);
	Assert(hr == S_OK, "failed to create device %x", hr);

	BOOL success = ctx->g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), 
		(void**)&ctx->g_d3dDebug);
	Assert(SUCCEEDED(success), "failed to get debug device");
	success = ctx->g_d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue),
		(void**)&ctx->g_d3dInfoQueue);
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
		ctx->g_d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
		ctx->g_d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	}

	ID3D11Texture2D* pBackBuffer;
	ctx->g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	// TODO: Should we not be using an SRGB conversion on the backbuffer? 
	//	From my understanding we should, but it looks visually wrong to me. 
	// D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	// rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &rtvDesc, &ctx->g_mainRenderTargetView);
	ctx->g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx->g_mainRenderTargetView);
	pBackBuffer->Release();
}

void Release(Context* ctx)
{
	SafeRelease(ctx->g_mainRenderTargetView);
	SafeRelease(ctx->g_pSwapChain);
	SafeRelease(ctx->g_pd3dDeviceContext);
	SafeRelease(ctx->g_d3dInfoQueue);
	SafeRelease(ctx->g_d3dDebug);
	SafeRelease(ctx->g_pd3dDevice);

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
}


Texture CreateTexture2D(Context* ctx, u32 w, u32 h, DXGI_FORMAT fmt, BindFlag flags)
{
	ID3D11Texture2D* D3dObject;
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = fmt;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = 0;
	if ((flags & BindFlag_SRV) != 0)
		desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if ((flags & BindFlag_UAV) != 0)
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	if ((flags & BindFlag_RTV) != 0)
		desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	if ((flags & BindFlag_DSV) != 0)
		desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	HRESULT hr = ctx->g_pd3dDevice->CreateTexture2D(&desc, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

	return Texture{ D3dObject };
}

ShaderResourceView CreateShaderResourceView(Context* ctx, Texture tex)
{
	ID3D11ShaderResourceView* D3dObject;
	HRESULT hr = ctx->g_pd3dDevice->CreateShaderResourceView(tex.D3dObject, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create srv, hr=%x", hr);
	return ShaderResourceView{ D3dObject };
}

UnorderedAccessView CreateUnorderedAccessView(Context* ctx, Texture tex)
{
	ID3D11UnorderedAccessView* D3dObject;
	HRESULT hr = ctx->g_pd3dDevice->CreateUnorderedAccessView(tex.D3dObject, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create uav, hr=%x", hr);
	return UnorderedAccessView{ D3dObject };
}

RenderTargetView CreateRenderTargetView(Context* ctx, Texture tex)
{
	ID3D11RenderTargetView* D3dObject;
	HRESULT hr = ctx->g_pd3dDevice->CreateRenderTargetView(tex.D3dObject, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create rtv, hr=%x", hr);
	return RenderTargetView{ D3dObject };
}

DepthStencilView CreateDepthStencilView(Context* ctx, Texture tex)
{
	ID3D11DepthStencilView* D3dObject;
	HRESULT hr = ctx->g_pd3dDevice->CreateDepthStencilView(tex.D3dObject, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create dsv, hr=%x", hr);
	return DepthStencilView{ D3dObject };
}



void Release(Texture& tex)
{
	SafeRelease(tex.D3dObject);
}

void Release(RenderTargetView& rtv)
{
	SafeRelease(rtv.D3dObject);
}

void Release(ShaderResourceView& srv)
{
	SafeRelease(srv.D3dObject);
}

void Release(UnorderedAccessView& uav)
{
	SafeRelease(uav.D3dObject);
}

void Release(DepthStencilView& dsv)
{
	SafeRelease(dsv.D3dObject);
}


void ClearRenderTarget(Context* ctx, RenderTargetView rtv, const float clear[4])
{
	ctx->g_pd3dDeviceContext->ClearRenderTargetView(rtv.D3dObject, clear);
}

void ClearDepth(Context* ctx, DepthStencilView dsv, float depth)
{
	ctx->g_pd3dDeviceContext->ClearDepthStencilView(dsv.D3dObject, 
		D3D11_CLEAR_DEPTH, depth, 0);
}

void ClearBackBufferRtv(Context* ctx)
{
	const float clear_0[4] = {0,0,0,0};
	ctx->g_pd3dDeviceContext->ClearRenderTargetView(ctx->g_mainRenderTargetView, clear_0);

}

void BindBackBufferRtv(Context* ctx)
{
	ctx->g_pd3dDeviceContext->OMSetRenderTargets(1, &ctx->g_mainRenderTargetView, nullptr);
}

void Present(Context* ctx, u8 vblanks)
{
	ctx->g_pSwapChain->Present(vblanks, 0); // Present with vsync
}

void HandleBackBufferResize(Context* ctx, u32 w, u32 h)
{
	SafeRelease(ctx->g_mainRenderTargetView);

	ctx->g_pSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* pBackBuffer;
	ctx->g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	// TODO: Should we not be using an SRGB conversion on the backbuffer? 
	//	From my understanding we should, but it looks visually wrong to me. 
	// D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	// rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &rtvDesc, &ctx->g_mainRenderTargetView);
	ctx->g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx->g_mainRenderTargetView);
	pBackBuffer->Release();
}

bool CheckD3DValidation(gfx::Context* ctx, std::string& outMessage)
{
	UINT64 num = ctx->g_d3dInfoQueue->GetNumStoredMessages();
	for (u32 i = 0 ; i < num ; ++i)
	{
		size_t messageLength;
		HRESULT hr = ctx->g_d3dInfoQueue->GetMessage(i, nullptr, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageLength);
		ctx->g_d3dInfoQueue->GetMessage(i, message, &messageLength);
		Assert(hr == S_FALSE, "Failed to get message, hr=%x", hr);
		outMessage += message->pDescription;
		free(message);
	}
	ctx->g_d3dInfoQueue->ClearStoredMessages();

	return num > 0;
}



#undef SafeRelease

}