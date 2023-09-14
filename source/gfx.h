

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

namespace gfx {

	struct Context;
	struct Texture;
	struct RenderTargetView;
	struct ShaderResourceView;
	struct UnorderedAccessView;
	struct DepthStencilView;

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

	struct Context {
		ID3D11Device*				g_pd3dDevice;
		ID3D11Debug*				g_d3dDebug;
		ID3D11InfoQueue*			g_d3dInfoQueue;
		ID3D11DeviceContext*		g_pd3dDeviceContext;
		IDXGISwapChain*				g_pSwapChain;
		ID3D11RenderTargetView*		g_mainRenderTargetView;
	};

	struct RasterizerState {
		ID3D11RasterizerState* D3dObject;
	};
	struct DepthStencilState {
		ID3D11DepthStencilState* D3dObject;
	};
	struct BlendState {
		ID3D11BlendState* D3dObject;
	};
	struct ShaderReflection {
		ID3D11ShaderReflection* D3dObject;
	};
	struct ComputeShader {
		ID3D11ComputeShader* D3dObject;
	};
	struct VertexShader {
		ID3D11VertexShader* D3dObject;
	};
	struct InputLayout {
		ID3D11InputLayout* D3dObject;
	};
	struct PixelShader {
		ID3D11PixelShader* D3dObject;
	};
	struct Buffer {
		ID3D11Buffer* D3dObject;
	};
	struct Texture {
		ID3D11Texture2D* D3dObject;
	};
	struct SamplerState {
		ID3D11SamplerState* D3dObject;
	};
	struct ShaderResourceView {
		ID3D11ShaderResourceView* D3dObject;
	};
	struct UnorderedAccessView {
		ID3D11UnorderedAccessView* D3dObject;
	};
	struct RenderTargetView {
		ID3D11RenderTargetView* D3dObject;
	};
	struct DepthStencilView {
		ID3D11DepthStencilView* D3dObject;
	};

}