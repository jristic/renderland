

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
#else
	#error unimplemented
#endif

}