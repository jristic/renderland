
namespace rlf
{
	struct InitErrorState 
	{
		bool InitSuccess;
		bool InitWarning;
		ErrorInfo Info;
	};

	void InitD3D(
		ID3D11Device* device,
		ID3D11InfoQueue* infoQueue,
		RenderDescription* rd,
		uint2 displaySize,
		const char* workingDirectory,
		InitErrorState* errorState);

	void ReleaseD3D(
		RenderDescription* rd);

	void HandleTextureParametersChanged(
		ID3D11Device* device,
		RenderDescription* rd,
		uint2 displaySize,
		u32 changedFlags,
		InitErrorState* errorState);

	struct ExecuteContext
	{
		ID3D11DeviceContext* D3dCtx;
		ID3D11RenderTargetView* MainRtv;
		ID3D11UnorderedAccessView* MainRtUav;
		ID3D11DepthStencilView* DefaultDepthView;
		uint2 DisplaySize;
		float Time;
	};

	struct ExecuteErrorState
	{
		bool ExecuteSuccess;
		ErrorInfo Info;
	};

	void Execute(
		ExecuteContext* context,
		RenderDescription* rd,
		ExecuteErrorState* es);
}
