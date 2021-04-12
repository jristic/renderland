
namespace rlf
{
	struct DispatchCompute
	{
		const char* ShaderPath;
		ID3D11ComputeShader* ShaderObject;
	};
	struct RenderDescription
	{
		std::vector<DispatchCompute*> Passes;
		std::set<DispatchCompute*> Dispatches;
		std::set<std::string> Strings;
	};

	RenderDescription* ParseBuffer(
		char* buffer,
		int buffer_len);

	void ReleaseData(RenderDescription* data);
}
