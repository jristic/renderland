
namespace rlf
{
	struct InitErrorState 
	{
		bool InitSuccess;
		bool InitWarning;
		std::string ErrorMessage;
	};

	void InitD3D(
		ID3D11Device* device,
		RenderDescription* rd,
		const char* workingDirectory,
		InitErrorState* errorState);

	void ReleaseD3D(
		RenderDescription* rd);

	struct ExecuteContext
	{
		ID3D11DeviceContext* D3dCtx;
		ID3D11RenderTargetView* MainRtv;
		ID3D11UnorderedAccessView* MainRtUav;
		ID3D11Buffer* GlobalConstantBuffer;
		uint2 DisplaySize;
		float Time;
	};

	void Execute(
		ExecuteContext* context,
		RenderDescription* rd);
}
