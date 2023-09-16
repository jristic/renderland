
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
		createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &ctx->SwapChain, 
		&ctx->Device, &featureLevel, &ctx->DeviceContext);
	Assert(hr == S_OK, "failed to create device %x", hr);

	BOOL success = ctx->Device->QueryInterface(__uuidof(ID3D11Debug), 
		(void**)&ctx->Debug);
	Assert(SUCCEEDED(success), "failed to get debug device");
	success = ctx->Debug->QueryInterface(__uuidof(ID3D11InfoQueue),
		(void**)&ctx->InfoQueue);
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
		ctx->InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
		ctx->InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	}

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

void Release(Context* ctx)
{
	SafeRelease(ctx->BackBufferRtv);
	SafeRelease(ctx->SwapChain);
	SafeRelease(ctx->DeviceContext);
	SafeRelease(ctx->InfoQueue);
	SafeRelease(ctx->Debug);
	SafeRelease(ctx->Device);

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
	HRESULT hr = ctx->Device->CreateTexture2D(&desc, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create texture, hr=%x", hr);

	return D3dObject;
}

ShaderResourceView CreateShaderResourceView(Context* ctx, Texture tex)
{
	ID3D11ShaderResourceView* D3dObject;
	HRESULT hr = ctx->Device->CreateShaderResourceView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create srv, hr=%x", hr);
	return D3dObject;
}

UnorderedAccessView CreateUnorderedAccessView(Context* ctx, Texture tex)
{
	ID3D11UnorderedAccessView* D3dObject;
	HRESULT hr = ctx->Device->CreateUnorderedAccessView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create uav, hr=%x", hr);
	return D3dObject;
}

RenderTargetView CreateRenderTargetView(Context* ctx, Texture tex)
{
	ID3D11RenderTargetView* D3dObject;
	HRESULT hr = ctx->Device->CreateRenderTargetView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create rtv, hr=%x", hr);
	return RenderTargetView{ D3dObject };
}

DepthStencilView CreateDepthStencilView(Context* ctx, Texture tex)
{
	ID3D11DepthStencilView* D3dObject;
	HRESULT hr = ctx->Device->CreateDepthStencilView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create dsv, hr=%x", hr);
	return D3dObject;
}



void Release(Texture& tex)
{
	SafeRelease(tex);
}

void Release(RenderTargetView& rtv)
{
	SafeRelease(rtv);
}

void Release(ShaderResourceView& srv)
{
	SafeRelease(srv);
}

void Release(UnorderedAccessView& uav)
{
	SafeRelease(uav);
}

void Release(DepthStencilView& dsv)
{
	SafeRelease(dsv);
}


void BeginFrame(Context* ctx)
{
	// intentional do nothing
	(void)ctx;
}

void EndFrame(Context* ctx)
{
	// intentional do nothing
	(void)ctx;
}


void ClearRenderTarget(Context* ctx, RenderTargetView rtv, const float clear[4])
{
	ctx->DeviceContext->ClearRenderTargetView(rtv, clear);
}

void ClearDepth(Context* ctx, DepthStencilView dsv, float depth)
{
	ctx->DeviceContext->ClearDepthStencilView(dsv, 
		D3D11_CLEAR_DEPTH, depth, 0);
}

void ClearBackBufferRtv(Context* ctx)
{
	const float clear_0[4] = {0,0,0,0};
	ctx->DeviceContext->ClearRenderTargetView(ctx->BackBufferRtv, clear_0);
}

void BindBackBufferRtv(Context* ctx)
{
	ctx->DeviceContext->OMSetRenderTargets(1, &ctx->BackBufferRtv, nullptr);
}

void Present(Context* ctx, u8 vblanks)
{
	ctx->SwapChain->Present(vblanks, 0); // Present with vsync
}

void WaitForLastSubmittedFrame(Context* ctx)
{
	// intentional do nothing - d3d11 has no manual frame sync
	(void)ctx;
}

void HandleBackBufferResize(Context* ctx, u32 w, u32 h)
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



#undef SafeRelease

}