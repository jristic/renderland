
namespace gfx
{


#define SafeRelease(ref) do { if (ref) { ref->Release(); ref = nullptr; } } while (0);

void CheckHresult(HRESULT hr, const char* desc)
{
	Assert(hr == S_OK, "Failed to create %s, hr=%x", desc, hr);
}


void SetupBackBuffer(Context* ctx)
{
	for (UINT i = 0; i < Context::NUM_BACK_BUFFERS; i++)
	{
		ID3D12Resource* pBackBuffer = NULL;
		ctx->SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
		ctx->Device->CreateRenderTargetView(pBackBuffer, NULL, ctx->BackBufferDescriptor[i]);
		ctx->BackBufferResource[i] = pBackBuffer;
	}
}

void CleanupBackBuffer(Context* ctx)
{
	for (u32 i = 0; i < Context::NUM_BACK_BUFFERS; i++)
		SafeRelease(ctx->BackBufferResource[i]);
}


void Initialize(Context* ctx, HWND hwnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC1 sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = Context::NUM_BACK_BUFFERS;
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
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&ctx->Debug));
	CheckHresult(hr, "debug interface");
	ctx->Debug->EnableDebugLayer();

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
	hr = D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&ctx->Device));
	CheckHresult(hr, "device");

	hr = ctx->Device->QueryInterface(IID_PPV_ARGS(&ctx->InfoQueue));
	CheckHresult(hr, "debug info queue");
	if (IsDebuggerPresent())
	{
		ctx->InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		ctx->InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// ctx->InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = Context::NUM_BACK_BUFFERS;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 1;
		hr = ctx->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ctx->RtvDescHeap));
		CheckHresult(hr, "descriptor heap");

		SIZE_T rtvDescriptorSize = ctx->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ctx->RtvDescHeap->GetCPUDescriptorHandleForHeapStart();
		for (u32 i = 0; i < Context::NUM_BACK_BUFFERS; i++)
		{
			ctx->BackBufferDescriptor[i] = rtvHandle;
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = ctx->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ctx->SrvDescHeap));
		CheckHresult(hr, "descriptor heap");
	}

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 1;
		hr = ctx->Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&ctx->CommandQueue));
		CheckHresult(hr, "command queue");
	}

	for (UINT i = 0; i < Context::NUM_FRAMES_IN_FLIGHT; i++)
	{
		hr = ctx->Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
			IID_PPV_ARGS(&ctx->FrameContexts[i].CommandAllocator));
		CheckHresult(hr, "command allocator");
	}

	hr = ctx->Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
		ctx->FrameContexts[0].CommandAllocator, nullptr, 
		IID_PPV_ARGS(&ctx->CommandList));
	CheckHresult(hr, "command list");
	ctx->CommandList->Close();

	hr = ctx->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx->Fence));
	CheckHresult(hr, "fence");

	ctx->FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	Assert(ctx->FenceEvent, "Failed to create fence event");

	{
		IDXGIFactory4* dxgiFactory = nullptr;
		IDXGISwapChain1* swapChain1 = nullptr;
		hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));
		CheckHresult(hr, "dxgi factory");
		hr = dxgiFactory->CreateSwapChainForHwnd(ctx->CommandQueue, hwnd, &sd, 
			nullptr, nullptr, &swapChain1);
		CheckHresult(hr, "swap chain");
		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&ctx->SwapChain));
		CheckHresult(hr, "swap chain");
		swapChain1->Release();
		dxgiFactory->Release();
		ctx->SwapChain->SetMaximumFrameLatency(Context::NUM_BACK_BUFFERS);
		ctx->SwapChainWaitableObject = ctx->SwapChain->GetFrameLatencyWaitableObject();
	}

	SetupBackBuffer(ctx);
}

void Release(Context* ctx)
{
	CleanupBackBuffer(ctx);

	ctx->SwapChain->SetFullscreenState(false, NULL);
	CloseHandle(ctx->SwapChainWaitableObject); ctx->SwapChainWaitableObject = nullptr;
	SafeRelease(ctx->SwapChain);
	for (u32 i = 0; i < Context::NUM_FRAMES_IN_FLIGHT; ++i)
		SafeRelease(ctx->FrameContexts[i].CommandAllocator);
	SafeRelease(ctx->CommandQueue);
	SafeRelease(ctx->CommandList);
	SafeRelease(ctx->RtvDescHeap);
	SafeRelease(ctx->SrvDescHeap);
	SafeRelease(ctx->Fence);
	CloseHandle(ctx->FenceEvent); ctx->FenceEvent = nullptr;
	// SafeRelease(ctx->InfoQueue);
	SafeRelease(ctx->Device);
	// SafeRelease(ctx->Debug);

#ifdef _DEBUG
	// Check for leaked D3D/DXGI objects
	IDXGIDebug1* pDebug = NULL;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif
}


Texture CreateTexture2D(Context* ctx, u32 w, u32 h, DXGI_FORMAT fmt, BindFlag flags)
{
/* ---------TODO---------

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
*/
	return nullptr;
}

ShaderResourceView CreateShaderResourceView(Context* ctx, Texture tex)
{
/* ---------TODO---------

	ID3D11ShaderResourceView* D3dObject;
	HRESULT hr = ctx->Device->CreateShaderResourceView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create srv, hr=%x", hr);
	return D3dObject;
*/
	return nullptr;
}

UnorderedAccessView CreateUnorderedAccessView(Context* ctx, Texture tex)
{
/* ---------TODO---------
	ID3D11UnorderedAccessView* D3dObject;
	HRESULT hr = ctx->Device->CreateUnorderedAccessView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create uav, hr=%x", hr);
	return D3dObject;
*/	
	return nullptr;
}

RenderTargetView CreateRenderTargetView(Context* ctx, Texture tex)
{
/* ---------TODO---------
	ID3D11RenderTargetView* D3dObject;
	HRESULT hr = ctx->Device->CreateRenderTargetView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create rtv, hr=%x", hr);
	return RenderTargetView{ D3dObject };
*/
	return nullptr;
}

DepthStencilView CreateDepthStencilView(Context* ctx, Texture tex)
{
/* ---------TODO---------
	ID3D11DepthStencilView* D3dObject;
	HRESULT hr = ctx->Device->CreateDepthStencilView(tex, nullptr, &D3dObject);
	Assert(hr == S_OK, "failed to create dsv, hr=%x", hr);
	return D3dObject;
*/
	return nullptr;
}



void Release(Texture& tex)
{
/* ---------TODO---------
	SafeRelease(tex);
*/
}

/* ---------TODO---------
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
*/


void BeginFrame(Context* ctx)
{
	HANDLE waitableObjects[] = { ctx->SwapChainWaitableObject, NULL };
	DWORD numWaitableObjects = 1;

	Context::Frame* frameCtx = &ctx->FrameContexts[ctx->FrameIndex % Context::NUM_FRAMES_IN_FLIGHT];
	UINT64 fenceValue = frameCtx->FenceValue;
	if (fenceValue != 0) // means no fence was signaled
	{
		frameCtx->FenceValue = 0;
		ctx->Fence->SetEventOnCompletion(fenceValue, ctx->FenceEvent);
		waitableObjects[1] = ctx->FenceEvent;
		numWaitableObjects = 2;
	}

	WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

	u32 backBufferIdx = ctx->SwapChain->GetCurrentBackBufferIndex();
	frameCtx->CommandAllocator->Reset();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = ctx->BackBufferResource[backBufferIdx];
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ctx->CommandList->Reset(frameCtx->CommandAllocator, NULL);
	ctx->CommandList->ResourceBarrier(1, &barrier);
}

void EndFrame(Context* ctx)
{
	// EndFrame is the last step before present, so transition the RT back to a 
	//	presentable surface.
	u32 backBufferIdx = ctx->SwapChain->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = ctx->BackBufferResource[backBufferIdx];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	ctx->CommandList->ResourceBarrier(1, &barrier);
	ctx->CommandList->Close();
	// Execute all recorded commands. 
	ctx->CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&ctx->CommandList);
}


void ClearRenderTarget(Context* ctx, RenderTargetView rtv, const float clear[4])
{
/* ---------TODO---------
	ctx->DeviceContext->ClearRenderTargetView(rtv, clear);
*/
}

void ClearDepth(Context* ctx, DepthStencilView dsv, float depth)
{
/* ---------TODO---------
	ctx->DeviceContext->ClearDepthStencilView(dsv, 
		D3D11_CLEAR_DEPTH, depth, 0);
*/
}

void ClearBackBufferRtv(Context* ctx)
{
	const float clear_0[4] = {0,0,0,0};
	u32 backBufferIdx = ctx->SwapChain->GetCurrentBackBufferIndex();
	ctx->CommandList->ClearRenderTargetView(ctx->BackBufferDescriptor[backBufferIdx], 
		clear_0, 0, nullptr);
}

void BindBackBufferRtv(Context* ctx)
{
	u32 backBufferIdx = ctx->SwapChain->GetCurrentBackBufferIndex();
	ctx->CommandList->OMSetRenderTargets(1, &ctx->BackBufferDescriptor[backBufferIdx], 
		FALSE, NULL);
	ctx->CommandList->SetDescriptorHeaps(1, &ctx->SrvDescHeap);
}

void Present(Context* ctx, u8 vblanks)
{
	ctx->SwapChain->Present(vblanks, 0);

	UINT64 fenceValue = ctx->FenceLastSignaledValue + 1;
	ctx->CommandQueue->Signal(ctx->Fence, fenceValue);
	ctx->FenceLastSignaledValue = fenceValue;

	ctx->FrameContexts[ctx->FrameIndex % Context::NUM_FRAMES_IN_FLIGHT].FenceValue = fenceValue;
	++ctx->FrameIndex;
}

void WaitForLastSubmittedFrame(Context* ctx)
{
	Assert(ctx->FrameIndex > 0, "At least one frame must be submitted.");
	Context::Frame* frameCtx = 
		&ctx->FrameContexts[(ctx->FrameIndex-1) % Context::NUM_FRAMES_IN_FLIGHT];

	u64 fenceValue = frameCtx->FenceValue;
	if (fenceValue == 0)
		return; // No fence was signaled

	frameCtx->FenceValue = 0;
	if (ctx->Fence->GetCompletedValue() >= fenceValue)
		return;

	ctx->Fence->SetEventOnCompletion(fenceValue, ctx->FenceEvent);
	WaitForSingleObject(ctx->FenceEvent, INFINITE);
}

void HandleBackBufferResize(Context* ctx, u32 w, u32 h)
{
	WaitForLastSubmittedFrame(ctx);
	CleanupBackBuffer(ctx);

	HRESULT hr = ctx->SwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
	Assert(hr == S_OK, "Failed to resize swapchain, hr=%x", hr);

	SetupBackBuffer(ctx);
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
		free(message);
	}
	ctx->InfoQueue->ClearStoredMessages();

	return num > 0;
}



#undef SafeRelease

}