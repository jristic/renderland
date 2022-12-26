
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

	struct ExecuteContext
	{
		ID3D11DeviceContext* D3dCtx;
		ID3D11RenderTargetView* MainRtv;
		ID3D11UnorderedAccessView* MainRtUav;
		ID3D11DepthStencilView* DefaultDepthView;
		ast::EvaluationContext EvCtx;
	};

	void HandleTextureParametersChanged(
		ID3D11Device* device,
		RenderDescription* rd,
		ExecuteContext* ec,
		ErrorState* errorState);

	void Execute(
		ExecuteContext* context,
		RenderDescription* rd,
		ErrorState* errorState);
}
