
#include "rlf/error.h"
#include "rlf/types.h"
#include "rlf/textureformat.h"
#include "rlf/ast.h"

// forward declares
namespace rlf 
{
	struct View;
}

namespace rlf
{
	enum class ResourceType
	{
		Invalid,
		Buffer,
		Texture,
	};
	enum class BindType
	{
		Invalid,
		SystemValue,
		Sampler,
		View,
	};
	enum class PassType
	{
		Invalid,
		Draw,
		Dispatch,
		ClearColor,
		ClearDepth,
		ClearStencil,
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
		BufferFlag_Structured = 4,
		BufferFlag_IndirectArgs = 8,
		BufferFlag_Raw = 16,
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
	enum class ComparisonFunc
	{
		Invalid,
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
	};
	enum class StencilOp
	{
		Invalid,
		Keep,
		Zero,
		Replace,
		IncrSat,
		DecrSat,
		Invert,
		Incr,
		Decr,
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
	struct StencilOpDesc
	{
		StencilOp StencilFailOp;
		StencilOp StencilDepthFailOp;
		StencilOp StencilPassOp;
		ComparisonFunc StencilFunc;
	};
	struct DepthStencilState
	{
		bool DepthEnable;
		bool DepthWrite;
		ComparisonFunc DepthFunc;
		bool StencilEnable;
		u8 StencilReadMask;
		u8 StencilWriteMask;
		StencilOpDesc FrontFace;
		StencilOpDesc BackFace;
		ID3D11DepthStencilState* DSSObject;
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
	};
	struct Texture
	{
		uint2 Size;
		ast::Node* SizeExpr;
		TextureFormat Format;
		const char* DDSPath;
		TextureFlag Flags;
		std::set<View*> Views;
		ID3D11Texture2D* TextureObject;
	};
	struct Sampler
	{
		FilterMode Filter;
		AddressModeUVW AddressMode;
		float MipLODBias;
		u32 MaxAnisotropy;
		float4 BorderColor;
		float MinLOD;
		float MaxLOD;
		ID3D11SamplerState* SamplerObject;
	};
	enum class ViewType 
	{
		Auto,
		SRV,
		UAV,
		RTV,
		DSV,
	};
	struct View
	{
		ViewType Type;
		ResourceType ResourceType;
		union {
			Buffer* Buffer;
			Texture* Texture;
		};
		TextureFormat Format;
		u32 NumElements;
		union {
			ID3D11ShaderResourceView* SRVObject;
			ID3D11UnorderedAccessView* UAVObject;
			ID3D11RenderTargetView* RTVObject;
			ID3D11DepthStencilView* DSVObject;
		};
	};
	struct Bind
	{
		const char* BindTarget;
		BindType Type;
		union {
			SystemValue SystemBind;
			Sampler* SamplerBind;
			View* ViewBind;
		};
		bool IsOutput;
		u32 BindIndex;
	};
	struct ConstantBuffer
	{
		u8* BackingMemory;
		ID3D11Buffer* BufferObject;
		std::string Name;
		u32 Slot;
		u32 Size;
	};
	struct SetConstant
	{
		const char* VariableName;
		ast::Node* Value;
		ConstantBuffer* CB;
		u32 Offset;
		u32 Size;
		VariableType Type;
	};
	struct Dispatch
	{
		ComputeShader* Shader;
		bool ThreadPerPixel;
		uint3 Groups;
		bool Indirect;
		Buffer* IndirectArgs;
		u32 IndirectArgsOffset;
		std::vector<Bind> Binds;
		std::vector<SetConstant> Constants;
		std::vector<ConstantBuffer> CBs;
	};
	struct TextureTarget
	{
		bool IsSystem;
		union {
			SystemValue System;
			View* View;
		};
	};
	struct Draw
	{
		DrawType Type;
		Topology Topology;
		RasterizerState* RState;
		DepthStencilState* DSState;
		VertexShader* VShader;
		PixelShader* PShader;
		Buffer* VertexBuffer;
		Buffer* IndexBuffer;
		u32 VertexCount;
		u8 StencilRef;
		std::vector<TextureTarget> RenderTarget;
		std::vector<TextureTarget> DepthStencil;
		std::vector<Bind> VSBinds;
		std::vector<Bind> PSBinds;
		std::vector<SetConstant> VSConstants;
		std::vector<SetConstant> PSConstants;
		std::vector<ConstantBuffer> VSCBs;
		std::vector<ConstantBuffer> PSCBs;
	};
	struct ClearColor
	{
		View* Target;
		float4 Color;
	};
	struct ClearDepth
	{
		View* Target;
		float Depth;
	};
	struct ClearStencil
	{
		View* Target;
		u8 Stencil;
	};
	struct Pass
	{
		const char* Name;
		PassType Type;
		union {
			Dispatch* Dispatch;
			Draw* Draw;
			ClearColor* ClearColor;
			ClearDepth* ClearDepth;
			ClearStencil* ClearStencil;
		};
	};
	struct Constant
	{
		const char* Name;
		ast::Node* Expr;
		VariableType Type;
		Variable Value;
	};
	struct Tuneable
	{
		const char* Name;
		VariableType Type;
		Variable Value;
		Variable Min;
		Variable Max;
	};
	struct RenderDescription
	{
		std::vector<Pass> Passes;
		std::vector<Dispatch*> Dispatches;
		std::vector<Draw*> Draws;
		std::vector<ClearColor*> ClearColors;
		std::vector<ClearDepth*> ClearDepths;
		std::vector<ClearStencil*> ClearStencils;
		std::vector<ComputeShader*> CShaders;
		std::vector<VertexShader*> VShaders;
		std::vector<PixelShader*> PShaders;
		std::vector<Buffer*> Buffers;
		std::vector<Texture*> Textures;
		std::vector<Sampler*> Samplers;
		std::vector<View*> Views;
		std::vector<RasterizerState*> RasterizerStates;
		std::vector<DepthStencilState*> DepthStencilStates;
		std::vector<ObjImport*> Objs;
		std::vector<Constant*> Constants;
		std::vector<Tuneable*> Tuneables;
		std::set<std::string> Strings;
		std::vector<void*> Mems;
		std::vector<ast::Node*> Asts;
	};
}
