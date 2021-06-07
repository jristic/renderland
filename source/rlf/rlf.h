
#include "rlf/textureformat.h"

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
	enum class PassType
	{
		Draw,
		Dispatch,
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
	enum class Topology
	{
		PointList,
		LineList,
		LineStrip,
		TriList,
		TriStrip,
	};
	enum class DrawType
	{
		Draw,
		DrawIndexed,
	};
	enum BufferFlags
	{
		BufferFlags_Vertex = 1,
		BufferFlags_Index = 2,
	};
	enum class CullMode
	{
		None,
		Front,
		Back,
	};
	struct RasterizerState 
	{
		bool Fill;
		CullMode CullMode;
		bool FrontCCW;
		i32 DepthBias;
		float SlopeScaledDepthBias;
		float DepthBiasClamp;
		bool DepthClipEnable;
		bool ScissorEnable;
		// TODO: MSAA support
		// bool MultisampleEnable;
		// bool AntialiasedLineEnable;
		ID3D11RasterizerState* RSObject;
	};
	struct ComputeShader
	{
		const char* ShaderPath;
		const char* EntryPoint;
		ID3D11ComputeShader* ShaderObject;
		ID3D11ShaderReflection* Reflector;
		uint3 ThreadGroupSize;
	};
	struct VertexShader
	{
		const char* ShaderPath;
		const char* EntryPoint;
		ID3D11VertexShader* ShaderObject;
		ID3D11ShaderReflection* Reflector;
		ID3D11InputLayout* InputLayout; 
	};
	struct PixelShader
	{
		const char* ShaderPath;
		const char* EntryPoint;
		ID3D11PixelShader* ShaderObject;
		ID3D11ShaderReflection* Reflector;
	};
	struct ObjImport 
	{
		const char* ObjPath;
		void* Vertices;
		u32 VertexCount;
		void* Indices;
		u32 IndexCount;
		bool U16;
	};
	struct Buffer
	{
		u32 ElementSize;
		u32 ElementCount;
		void* InitData;
		BufferFlags Flags;
		ID3D11Buffer* BufferObject;
		ID3D11ShaderResourceView* SRV;
		ID3D11UnorderedAccessView* UAV;
	};
	struct Texture
	{
		uint2 Size;
		TextureFormat Format;
		const char* DDSPath;
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
	struct Draw
	{
		DrawType Type;
		Topology Topology;
		RasterizerState* RState;
		VertexShader* VShader;
		PixelShader* PShader;
		Buffer* VertexBuffer;
		Buffer* IndexBuffer;
		u32 VertexCount;
		SystemValue RenderTarget;
		std::vector<Bind> VSBinds;
		std::vector<Bind> PSBinds;
	};
	struct Pass
	{
		PassType Type;
		union {
			Dispatch* Dispatch;
			Draw* Draw;
		};
	};
	struct RenderDescription
	{
		std::vector<Pass> Passes;
		std::vector<Dispatch*> Dispatches;
		std::vector<Draw*> Draws;
		std::vector<ComputeShader*> CShaders;
		std::vector<VertexShader*> VShaders;
		std::vector<PixelShader*> PShaders;
		std::vector<Buffer*> Buffers;
		std::vector<Texture*> Textures;
		std::vector<Sampler*> Samplers;
		std::vector<RasterizerState*> RasterizerStates;
		std::vector<ObjImport*> Objs;
		std::set<std::string> Strings;
		std::vector<void*> Mems;
	};
}
