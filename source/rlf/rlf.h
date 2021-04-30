
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
	enum class BindType
	{
		Invalid,
		SystemValue,
		Buffer,
		Texture,
	};
	enum class SystemValue
	{
		Invalid,
		BackBuffer,
	};
	struct Buffer
	{
		u32 ElementSize;
		u32 ElementCount;
		bool InitToZero;
		ID3D11Buffer* BufferObject;
		ID3D11ShaderResourceView* SRV;
		ID3D11UnorderedAccessView* UAV;
	};
	struct Bind
	{
		const char* BindTarget;
		bool IsSystemValue;
		union {
			SystemValue SystemBind;
			Buffer* BufferBind;
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
		std::vector<Buffer*> Buffers;
	};
}
