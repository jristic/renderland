
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
		Invalid,
		Draw,
		Dispatch,
		ClearColor,
		ClearDepth,
	};
	enum class SystemValue
	{
		Invalid,
		BackBuffer,
		DefaultDepth,
	};
	enum class Filter
	{
		Invalid,
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
		Invalid,
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
		Invalid,
		PointList,
		LineList,
		LineStrip,
		TriList,
		TriStrip,
	};
	enum class DrawType
	{
		Invalid,
		Draw,
		DrawIndexed,
	};
	enum BufferFlag
	{
		BufferFlag_Vertex = 1,
		BufferFlag_Index = 2,
	};
	enum TextureFlag
	{
		TextureFlag_SRV = 1,
		TextureFlag_UAV = 2,
		TextureFlag_RTV = 4,
		TextureFlag_DSV = 8,
	};
	enum class CullMode
	{
		Invalid,
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
		BufferFlag Flags;
		ID3D11Buffer* BufferObject;
		ID3D11ShaderResourceView* SRV;
		ID3D11UnorderedAccessView* UAV;
	};
	struct Texture
	{
		uint2 Size;
		TextureFormat Format;
		const char* DDSPath;
		TextureFlag Flags;
		ID3D11Texture2D* TextureObject;
		ID3D11ShaderResourceView* SRV;
		ID3D11UnorderedAccessView* UAV;
		ID3D11RenderTargetView* RTV;
		ID3D11DepthStencilView* DSV;
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
	struct TextureTarget
	{
		BindType Type;
		union {
			SystemValue System;
			Texture* Texture;
		};
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
		TextureTarget RenderTarget;
		TextureTarget DepthStencil;
		std::vector<Bind> VSBinds;
		std::vector<Bind> PSBinds;
	};
	struct ClearColor
	{
		Texture* Target;
		float4 Color;
	};
	struct ClearDepth
	{
		Texture* Target;
		float Depth;
	};
	struct Pass
	{
		PassType Type;
		union {
			Dispatch* Dispatch;
			Draw* Draw;
			ClearColor* ClearColor;
			ClearDepth* ClearDepth;
		};
	};
	struct RenderDescription
	{
		std::vector<Pass> Passes;
		std::vector<Dispatch*> Dispatches;
		std::vector<Draw*> Draws;
		std::vector<ClearColor*> ClearColors;
		std::vector<ClearDepth*> ClearDepths;
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
