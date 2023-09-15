

namespace gfx {

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