

// forward declares, so we don't need to include the d3d headers
#if D3D11
	struct ID3D11Device;
	struct ID3D11Debug;
	struct ID3D11InfoQueue;
	struct ID3D11DeviceContext;
	struct IDXGISwapChain;
	struct ID3D11RenderTargetView;
	struct ID3D11Texture2D;
	struct ID3D11RenderTargetView;
	struct ID3D11ShaderResourceView;
	struct ID3D11UnorderedAccessView;
	struct ID3D11Texture2D;
	struct ID3D11DepthStencilView;
	struct ID3D11RasterizerState;
	struct ID3D11DepthStencilState;
	struct ID3D11BlendState;
	struct ID3D11ShaderReflection;
	struct ID3D11ComputeShader;
	struct ID3D11VertexShader;
	struct ID3D11InputLayout;
	struct ID3D11PixelShader;
	struct ID3D11Buffer;
	struct ID3D11Texture2D;
	struct ID3D11SamplerState;
	struct ID3D11ShaderResourceView;
	struct ID3D11UnorderedAccessView;
	struct ID3D11RenderTargetView;
	struct ID3D11DepthStencilView;
#elif D3D12
	struct ID3D12CommandAllocator;
	struct ID3D12Device;
	struct ID3D12InfoQueue;
	struct ID3D12DescriptorHeap;
	struct ID3D12CommandQueue;
	struct ID3D12GraphicsCommandList;
	struct ID3D12Fence;
	struct IDXGISwapChain3;
	struct ID3D12Resource;
	struct D3D12_CPU_DESCRIPTOR_HANDLE;
	struct D3D12_GPU_DESCRIPTOR_HANDLE;
#else
	#error unimplemented
#endif

namespace gfx {

	struct Context {
#if D3D11
		ID3D11Device*				Device;
		ID3D11Debug*				Debug;
		ID3D11InfoQueue*			InfoQueue;
		ID3D11DeviceContext*		DeviceContext;
		IDXGISwapChain*				SwapChain;
		ID3D11RenderTargetView*		BackBufferRtv;
#elif D3D12
		struct Frame
		{
			ID3D12CommandAllocator* CommandAllocator;
			u64 FenceValue;
		};

		static u32 const				NUM_FRAMES_IN_FLIGHT = 3;
		Frame							FrameContexts[NUM_FRAMES_IN_FLIGHT] = {};
		u32								FrameIndex = 0;

		static u32 const				NUM_BACK_BUFFERS = 3;
		ID3D12Device*					Device = nullptr;
		ID3D12Debug*					Debug = nullptr;
		ID3D12InfoQueue*				InfoQueue = nullptr;
		ID3D12DescriptorHeap*			RtvDescHeap = nullptr;
		ID3D12DescriptorHeap*			SrvDescHeap = nullptr;
		ID3D12CommandQueue*				CommandQueue = nullptr;
		ID3D12GraphicsCommandList*		CommandList = nullptr;
		ID3D12Fence*					Fence = nullptr;
		HANDLE							FenceEvent = nullptr;
		u64								FenceLastSignaledValue = 0;
		IDXGISwapChain3*				SwapChain = nullptr;
		HANDLE							SwapChainWaitableObject = nullptr;
		ID3D12Resource*					BackBufferResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE		BackBufferDescriptor[NUM_BACK_BUFFERS] = {};
#else
	#error unimplemented
#endif
	};

#if D3D11
	typedef ID3D11RasterizerState* RasterizerState;
	typedef ID3D11DepthStencilState* DepthStencilState;
	typedef ID3D11BlendState* BlendState;
	typedef ID3D11ShaderReflection* ShaderReflection;
	typedef ID3D11ComputeShader* ComputeShader;
	typedef ID3D11VertexShader* VertexShader;
	typedef ID3D11InputLayout* InputLayout;
	typedef ID3D11PixelShader* PixelShader;
	typedef ID3D11Buffer* Buffer;
	typedef ID3D11Texture2D* Texture;
	typedef ID3D11SamplerState* SamplerState;
	typedef ID3D11ShaderResourceView* ShaderResourceView;
	typedef ID3D11UnorderedAccessView* UnorderedAccessView;
	typedef ID3D11RenderTargetView* RenderTargetView;
	typedef ID3D11DepthStencilView* DepthStencilView;
#elif D3D12
	typedef void* RasterizerState;
	typedef void* DepthStencilState;
	typedef void* BlendState;
	typedef void* ShaderReflection;
	typedef void* ComputeShader;
	typedef void* VertexShader;
	typedef void* InputLayout;
	typedef void* PixelShader;
	typedef void* Buffer;
	typedef void* Texture;
	typedef void* SamplerState;
	typedef void* ShaderResourceView;
	typedef void* UnorderedAccessView;
	typedef void* RenderTargetView;
	typedef void* DepthStencilView;

#else
	#error unimplemented
#endif

}