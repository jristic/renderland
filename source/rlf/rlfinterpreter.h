
namespace rlf
{
	void InitD3D(
		ID3D11Device* device,
		ID3D11InfoQueue* infoQueue,
		RenderDescription* rd,
		uint2 displaySize,
		const char* workingDirectory,
		ErrorState* errorState);

	void ReleaseD3D(
		RenderDescription* rd);

	void HandleTextureParametersChanged(
		ID3D11Device* device,
		RenderDescription* rd,
		uint2 displaySize,
		u32 changedFlags,
		ErrorState* errorState);

	struct ExecuteContext
	{
		ID3D11DeviceContext* D3dCtx;
		ID3D11RenderTargetView* MainRtv;
		ID3D11UnorderedAccessView* MainRtUav;
		ID3D11DepthStencilView* DefaultDepthView;
		uint2 DisplaySize;
		float Time;
	};

	void Execute(
		ExecuteContext* context,
		RenderDescription* rd,
		ErrorState* errorState);
}
