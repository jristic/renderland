

namespace gfx {

	struct DescriptorHeap {
		ID3D12DescriptorHeap* 	Object;
		u64						DescriptorSize;
		u64						MaxSlots;
		u64						ReservedSlots;
		u64						NextIndex;
	};

	struct Context {
		struct Frame
		{
			ID3D12CommandAllocator* CommandAllocator;
			u64 FenceValue;
		};

		static u32 const				NUM_FRAMES_IN_FLIGHT = 2;
		Frame							FrameContexts[NUM_FRAMES_IN_FLIGHT] = {};
		u32								FrameIndex = 0;

		static u32 const				NUM_BACK_BUFFERS = 2;
		ID3D12Device*					Device = nullptr;
		ID3D12Debug*					Debug = nullptr;
		ID3D12InfoQueue*				InfoQueue = nullptr;

		// Descriptor heaps
		static u32 const				MAX_RTV_DESCS = 512;
		static u32 const 				RLF_RESERVED_RTV_SLOT_INDEX = NUM_BACK_BUFFERS;
		static u32 const 				NUM_RESERVED_RTV_SLOTS = NUM_BACK_BUFFERS + 1;

		static u32 const				MAX_CBV_SRV_UAV_DESCS = 2048;
		static u32 const				RLF_RESERVED_SRV_SLOT_INDEX = 0;
		static u32 const				RLF_RESERVED_UAV_SLOT_INDEX = 1;
		static u32 const 				NUM_RESERVED_CBV_SRV_UAV_SLOTS = 2;

		static u32 const				MAX_DSV_DESCS = 256;
		static u32 const 				NUM_RESERVED_DSV_SLOTS = 0;

		static u32 const				MAX_SHADER_VIS_DESCS = 2048;
		static u32 const 				IMGUI_FONT_RESERVED_SRV_SLOT_INDEX = 0;
		static u32 const				RLF_RESERVED_SHADER_VIS_SLOT_INDEX = 1;
		static u32 const 				NUM_RESERVED_SHADER_VIS_SLOTS = 2;

		DescriptorHeap		RtvHeap;
		DescriptorHeap		DsvHeap;
		DescriptorHeap		CbvSrvUavCreationHeap;
		DescriptorHeap		CbvSrvUavHeap;
		DescriptorHeap		SamplerCreationHeap;
		DescriptorHeap		SamplerHeap;


		ID3D12CommandQueue*				CommandQueue = nullptr;
		ID3D12GraphicsCommandList*		CommandList = nullptr;
		ID3D12Fence*					Fence = nullptr;
		HANDLE							FenceEvent = nullptr;
		u64								FenceLastSignaledValue = 0;
		IDXGISwapChain3*				SwapChain = nullptr;
		HANDLE							SwapChainWaitableObject = nullptr;
		ID3D12Resource*					BackBufferResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE		BackBufferDescriptor[NUM_BACK_BUFFERS] = {};

		static u32 const 				UPLOAD_BUFFER_SIZE = 100*1024*1024;
		ID3D12Resource*					UploadBufferResource = nullptr;
		void* 							UploadBufferMem = nullptr;
		ID3D12GraphicsCommandList*		UploadCommandList = nullptr;
	};

	struct BindInfo {
		u32 NumCbvs;
		u32 NumSrvs;
		u32 NumUavs;
		u32 NumSamplers;
		u32 CbvMin;
		u32 CbvMax;
		u32 SrvMin;
		u32 SrvMax;
		u32 UavMin;
		u32 UavMax;
		u32 SamplerMin;
		u32 SamplerMax;
	};

	struct RasterizerState {};
	struct DepthStencilState {};
	struct BlendState {};
	typedef ID3D12ShaderReflection* ShaderReflection;
	struct ComputeShader {
		ID3DBlob* Blob;
		ID3D12PipelineState* Pipeline;
		ID3D12RootSignature* RootSig;
		BindInfo BI;
	};
	struct InputLayout {};
	struct VertexShader {
		ID3DBlob* Blob;
		BindInfo BI;
	};
	struct PixelShader {
		ID3DBlob* Blob;
		BindInfo BI;
	};
	struct ConstantBuffer {
		ID3D12Resource* Resource[Context::NUM_FRAMES_IN_FLIGHT];
		void* MappedMem[Context::NUM_FRAMES_IN_FLIGHT];
		D3D12_CPU_DESCRIPTOR_HANDLE CbvDescriptor[Context::NUM_FRAMES_IN_FLIGHT];
	};
	struct Buffer {
		D3D12_RESOURCE_STATES State;
		ID3D12Resource* Resource;
	};
	struct Texture {
		D3D12_RESOURCE_STATES State;
		ID3D12Resource* Resource;
	};
	typedef D3D12_CPU_DESCRIPTOR_HANDLE SamplerState;
	typedef D3D12_CPU_DESCRIPTOR_HANDLE ShaderResourceView;
	typedef D3D12_CPU_DESCRIPTOR_HANDLE UnorderedAccessView;
	typedef D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView;
	typedef D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView;

	struct DescriptorTable {
		u64 CbvSrvUavDescTableStart[Context::NUM_FRAMES_IN_FLIGHT];
		u64 SamplerDescTableStart[Context::NUM_FRAMES_IN_FLIGHT];
	};

	struct DispatchData {
		DescriptorTable Table;
		ID3D12CommandSignature* CommandSig;
	};
	struct DrawData {
		ID3D12RootSignature* RootSig;
		ID3D12PipelineState* Pipeline;
		DescriptorTable VSTable;
		DescriptorTable PSTable;
		ID3D12CommandSignature* CommandSig;
	};


	void CreateDescriptorHeap(gfx::Context* ctx, DescriptorHeap* heap, const wchar_t* name,
		D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, u32 max_slots, 
		u32 reserved_slots)
	{
		Assert(reserved_slots < max_slots, "Invalid parameters");
		Assert(heap->Object == nullptr, "Leaked object");

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = type;
		desc.NumDescriptors = max_slots;
		desc.Flags = flags;
		HRESULT hr = ctx->Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap->Object));
		heap->Object->SetName(name);
		Assert(hr == S_OK, "failed to create descriptor heap %s, hr=%x", hr);

		heap->DescriptorSize = ctx->Device->GetDescriptorHandleIncrementSize(type);
		heap->MaxSlots = max_slots;
		heap->ReservedSlots = heap->NextIndex = reserved_slots;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(DescriptorHeap* heap)
	{
		Assert(heap->NextIndex < heap->MaxSlots, "Ran out of descriptors");
		u64 offset = heap->NextIndex * heap->DescriptorSize;

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = 
			heap->Object->GetCPUDescriptorHandleForHeapStart();
		cpu_descriptor.ptr += offset;

		++heap->NextIndex;

		return cpu_descriptor;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptor(DescriptorHeap* heap, u64 slot)
	{
		Assert(slot < heap->MaxSlots, "Invalid slot");
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = 
			heap->Object->GetCPUDescriptorHandleForHeapStart();
		cpu_descriptor.ptr += slot * heap->DescriptorSize;
		return cpu_descriptor;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(DescriptorHeap* heap, u64 slot)
	{
		Assert(slot < heap->MaxSlots, "Invalid slot");
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor = 
			heap->Object->GetGPUDescriptorHandleForHeapStart();
		gpu_descriptor.ptr += slot * heap->DescriptorSize;
		return gpu_descriptor;
	}

	void ResetHeap(DescriptorHeap* heap)
	{
		heap->NextIndex = heap->ReservedSlots;
	}


	void TransitionResource(Context* ctx, Texture* tex, D3D12_RESOURCE_STATES state)
	{
		if (tex->State == state)
			return;
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = tex->Resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = tex->State;
		barrier.Transition.StateAfter  = state;
		ctx->CommandList->ResourceBarrier(1, &barrier);

		tex->State = state;
	}

	void TransitionResource(Context* ctx, Buffer* buf, D3D12_RESOURCE_STATES state)
	{
		if (buf->State == state)
			return;
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = buf->Resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = buf->State;
		barrier.Transition.StateAfter  = state;
		ctx->CommandList->ResourceBarrier(1, &barrier);
		
		buf->State = state;
	}

}
