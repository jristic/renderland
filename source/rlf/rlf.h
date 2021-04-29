
namespace rlf
{
	struct ComputeShader
	{
		const char* ShaderPath;
		const char* EntryPoint;
		ID3D11ComputeShader* ShaderObject;
		ID3D11ShaderReflection* Reflector;
		uint3 ThreadGroupSize;
	};
	enum class SystemValue {
		Invalid,
		BackBuffer,
	};
	struct Bind
	{
		const char* BindTarget;
		bool IsSystemValue;
		union {
			SystemValue SystemBind;
			const char* TextureBind;
		};
		bool IsOutput;
		u32 BindIndex;
	};
	struct Dispatch
	{
		ComputeShader* Shader;
		bool ThreadPerPixel;
		uint3 Groups;
		std::vector<Bind> Binds;
	};
	struct RenderDescription
	{
		std::vector<Dispatch*> Passes;
		std::vector<Dispatch*> Dispatches;
		std::vector<ComputeShader*> Shaders;
		std::set<std::string> Strings;
	};
}
