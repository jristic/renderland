

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

		// Descriptor heaps
		static u32 const				MAX_RTV_DESCS = 512;
		static u32 const 				RLF_RESERVED_RTV_SLOT_INDEX = NUM_BACK_BUFFERS;
		static u32 const 				NUM_RESERVED_RTV_SLOTS = NUM_BACK_BUFFERS + 1;
		ID3D12DescriptorHeap*			RtvDescHeap = nullptr;
		u64								RtvDescSize = 0;
		u64								RtvDescNextIndex = 0;
		static u32 const				MAX_SRV_UAV_DESCS = 1024;
		static u32 const				RLF_RESERVED_SRV_SLOT_INDEX = 0;
		static u32 const				RLF_RESERVED_UAV_SLOT_INDEX = 1;
		static u32 const 				NUM_RESERVED_SRV_UAV_SLOTS = 2;
		ID3D12DescriptorHeap*			SrvUavDescHeap = nullptr;
		u64								SrvUavDescSize = 0;
		u64								SrvUavDescNextIndex = 0;
		static u32 const				MAX_DSV_DESCS = 256;
		static u32 const				RLF_RESERVED_DSV_SLOT_INDEX = 0;
		static u32 const 				NUM_RESERVED_DSV_SLOTS = 1;
		ID3D12DescriptorHeap*			DsvDescHeap = nullptr;
		u64								DsvDescSize = 0;
		u64								DsvDescNextIndex = 0;
		static u32 const				MAX_SHADER_VIS_DESCS = 2048;
		static u32 const 				IMGUI_FONT_RESERVED_SRV_SLOT_INDEX = 0;
		static u32 const				RLF_RESERVED_SHADER_VIS_SLOT_INDEX = 1;
		static u32 const 				NUM_RESERVED_SHADER_VIS_SLOTS = 2;
		ID3D12DescriptorHeap*			ShaderVisDescHeap = nullptr;
		u64								ShaderVisDescNextIndex = 0;


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
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptor;
	};
	struct UnorderedAccessView {
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptor;
	};
	struct RenderTargetView  {
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
	};
	struct DepthStencilView  {
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor;
	};
}
