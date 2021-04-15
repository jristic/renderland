
namespace rlf
{
	struct ComputeShader
	{
		const char* ShaderPath;
		const char* EntryPoint;
		ID3D11ComputeShader* ShaderObject;
		uint3 ThreadGroupSize;
	};
	struct Dispatch
	{
		ComputeShader* Shader;
		bool ThreadPerPixel;
		uint3 Groups;
	};
	struct RenderDescription
	{
		std::vector<Dispatch*> Passes;
		std::vector<Dispatch*> Dispatches;
		std::vector<ComputeShader*> Shaders;
		std::set<std::string> Strings;
	};
}
