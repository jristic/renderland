

// forward declares, so we don't need to include the d3d11 headers
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

// forward declares for rlf types
// TODO: extract type definitions from this header to a gfx_types.h so rlf.h can
//	be included before gfx.h
namespace rlf {
	struct RasterizerState;
	struct DepthStencilState;
	struct ComputeShader;
	struct VertexShader;
	struct PixelShader;
	struct Buffer;
	struct Texture;
	struct Sampler;
	struct View;
	struct ConstantBuffer;
	struct Draw;
}

namespace gfx {

	struct Context {
		ID3D11Device*				Device;
		ID3D11Debug*				Debug;
		ID3D11InfoQueue*			InfoQueue;
		ID3D11DeviceContext*		DeviceContext;
		IDXGISwapChain*				SwapChain;
		ID3D11RenderTargetView*		BackBufferRtv;
	};

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

	enum BindFlag
	{
		BindFlag_SRV = 1,
		BindFlag_UAV = 2,
		BindFlag_RTV = 4,
		BindFlag_DSV = 8,
	};

	void Initialize(Context* ctx, HWND hwnd);
	void Release(Context* ctx);

	Texture CreateTexture2D(Context* ctx, u32 w, u32 h, DXGI_FORMAT fmt, BindFlag flags);
	ShaderResourceView CreateShaderResourceView(Context* ctx, Texture tex);
	UnorderedAccessView CreateUnorderedAccessView(Context* ctx, Texture tex);
	RenderTargetView CreateRenderTargetView(Context* ctx, Texture tex);
	DepthStencilView CreateDepthStencilView(Context* ctx, Texture tex);
	void Release(Texture& tex);
	void Release(RenderTargetView& tex);
	void Release(ShaderResourceView& tex);
	void Release(UnorderedAccessView& tex);
	void Release(DepthStencilView& tex);

	void ClearRenderTarget(Context* ctx, RenderTargetView rtv, const float clear[4]);
	void ClearDepth(Context* ctx, DepthStencilView dsv, float depth);

	void ClearBackBufferRtv(Context* ctx);
	void BindBackBufferRtv(Context* ctx);
	void Present(Context* ctx, u8 vblanks);

	void HandleBackBufferResize(Context* ctx, u32 w, u32 h);
	bool CheckD3DValidation(Context* ctx, std::string& outMessage);

}