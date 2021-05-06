
namespace rlf
{
	enum class BindType
	{
		Invalid,
		SystemValue,
		Buffer,
		Texture,
		Sampler,
	};
	enum class SystemValue
	{
		Invalid,
		BackBuffer,
	};
	enum class Filter
	{
		Point,
		Linear,
		Aniso,
	};
	struct FilterMode
	{
		Filter Min;
		Filter Mag;
		Filter Mip;
	};
	enum class AddressMode
	{
		Wrap,
		Mirror,
		MirrorOnce,
		Clamp,
		Border,
	};
	struct AddressModeUVW
	{
		AddressMode U,V,W;
	};
	struct ComputeShader
	{
		const char* ShaderPath;
		const char* EntryPoint;
		ID3D11ComputeShader* ShaderObject;
		ID3D11ShaderReflection* Reflector;
		uint3 ThreadGroupSize;
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
	struct Texture
	{
		uint2 Size;
		bool InitToZero;
		ID3D11Texture2D* TextureObject;
		ID3D11ShaderResourceView* SRV;
		ID3D11UnorderedAccessView* UAV;
	};
	struct Sampler
	{
		FilterMode Filter;
		AddressModeUVW Address;
		float MipLODBias;
		u32 MaxAnisotropy;
		float4 BorderColor;
		float MinLOD;
		float MaxLOD;
		ID3D11SamplerState* SamplerObject;
	};
	struct Bind
	{
		const char* BindTarget;
		BindType Type;
		union {
			SystemValue SystemBind;
			Buffer* BufferBind;
			Texture* TextureBind;
			Sampler* SamplerBind;
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
		std::vector<Texture*> Textures;
		std::vector<Sampler*> Samplers;
	};
}
