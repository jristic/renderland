

namespace gfx {

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
		static u32 const				MAX_RTV_DESCS = 512;
		ID3D12DescriptorHeap*			RtvDescHeap = nullptr;
		u64								RtvDescSize = 0;
		u64								RtvDescNextIndex = 0;
		static u32 const				MAX_SRV_DESCS = 1024;
		ID3D12DescriptorHeap*			SrvDescHeap = nullptr;
		u64								SrvDescSize = 0;
		u64								SrvDescNextIndex = 0;
		static u32 const				MAX_DSV_DESCS = 256;
		ID3D12DescriptorHeap*			DsvDescHeap = nullptr;
		u64								DsvDescSize = 0;
		u64								DsvDescNextIndex = 0;
		ID3D12CommandQueue*				CommandQueue = nullptr;
		ID3D12GraphicsCommandList*		CommandList = nullptr;
		ID3D12Fence*					Fence = nullptr;
		HANDLE							FenceEvent = nullptr;
		u64								FenceLastSignaledValue = 0;
		IDXGISwapChain3*				SwapChain = nullptr;
		HANDLE							SwapChainWaitableObject = nullptr;
		ID3D12Resource*					BackBufferResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE		BackBufferDescriptor[NUM_BACK_BUFFERS] = {};
	};

	typedef void* RasterizerState;
	typedef void* DepthStencilState;
	typedef void* BlendState;
	typedef ID3D12ShaderReflection* ShaderReflection;
	typedef ID3DBlob* ComputeShader;
	typedef void* VertexShader;
	typedef void* InputLayout;
	typedef void* PixelShader;
	typedef void* Buffer;
	struct Texture {
		ID3D12Resource* Resource;
	};
	typedef void* SamplerState;
	struct ShaderResourceView {
		D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptor;
	};
	struct UnorderedAccessView {
		D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptor;
	};
	struct RenderTargetView  {
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
	};
	struct DepthStencilView  {
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
	};
}
