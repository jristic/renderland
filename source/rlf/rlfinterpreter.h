
namespace rlf
{
	struct InitErrorState 
	{
		bool InitSuccess;
		std::string ErrorMessage;
	};

	void InitD3D(
		ID3D11Device* device,
		RenderDescription* rd,
		const char* workingDirectory,
		InitErrorState* errorState);

	void Execute(
		ID3D11DeviceContext* ctx,
		RenderDescription* rd,
		float time);
}
