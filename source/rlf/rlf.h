
#include "rlf/error.h"
#include "rlf/types.h"
#include "rlf/textureformat.h"
#include "rlf/alloc.h"
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
		Resolve,
		ObjDraw,
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
	enum class Blend
	{
		Invalid,
		Zero,
		One,
		SrcColor,
		InvSrcColor,
		SrcAlpha,
		InvSrcAlpha,
		DestAlpha,
		InvDestAlpha,
		DestColor,
		InvDestColor,
		SrcAlphaSat,
		BlendFactor,
		InvBlendFactor,
		Src1Color,
		InvSrc1Color,
		Src1Alpha,
		InvSrc1Alpha,
	};
	enum class BlendOp
	{
		Invalid,
		Add,
		Subtract,
		RevSubtract,
		Min,
		Max,
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
		bool MultisampleEnable;
		bool AntialiasedLineEnable;
		gfx::RasterizerState GfxState;
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
		gfx::DepthStencilState GfxState;
	};
	struct BlendState
	{
		bool Enable;
		Blend Src;
		Blend Dest;
		BlendOp Op;
		Blend SrcAlpha;
		Blend DestAlpha;
		BlendOp OpAlpha;
		u8 RenderTargetWriteMask;
	};
	enum class InputClassification
	{
		Invalid,
		PerVertex,
		PerInstance,
	};
	struct InputElementDesc
	{
		char* SemanticName;
		u32 SemanticIndex;
		TextureFormat Format;
		u32 InputSlot;
		u32 AlignedByteOffset;
		InputClassification InputSlotClass;
		u32 InstanceDataStepRate;
	};
	struct CommonShader 
	{
		const char* ShaderPath;
		const char* EntryPoint;
		gfx::ShaderReflection Reflector;
	};
	struct ComputeShader
	{
		CommonShader Common;
		gfx::ComputeShader GfxState;
		uint3 ThreadGroupSize;
	};
	struct VertexShader
	{
		CommonShader Common;
		Array<InputElementDesc> InputLayout;
		gfx::VertexShader GfxState;
		gfx::InputLayout LayoutGfxState; 
	};
	struct PixelShader
	{
		CommonShader Common;
		gfx::PixelShader GfxState;
	};
	struct SizeOfRequest
	{
		CommonShader* Shader;
		ast::SizeOf* Dest;
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
		ast::Expression ElementSizeExpr;
		u32 ElementCount;
		ast::Expression ElementCountExpr;
		bool InitToZero;
		void* InitData;
		u32 InitDataSize;
		BufferFlag Flags;
		gfx::Buffer GfxState;
	};
	struct Texture
	{
		uint2 Size;
		ast::Expression SizeExpr;
		TextureFormat Format;
		const char* FromFile;
		TextureFlag Flags;
		u32 SampleCount;
		gfx::Texture GfxState;
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
		gfx::SamplerState GfxState;
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
			gfx::ShaderResourceView SRVGfxState;
			gfx::UnorderedAccessView UAVGfxState;
			gfx::RenderTargetView RTVGfxState;
			gfx::DepthStencilView DSVGfxState;
		};
	};
	struct Viewport
	{
		ast::Expression TopLeft;
		ast::Expression Size;
		ast::Expression DepthRange;
	};
	struct Bind
	{
		const char* BindTarget;
		BindType Type;
		union {
			Sampler* SamplerBind;
			View* ViewBind;
		};
		bool IsOutput;
		u32 BindIndex;
	};
	struct ConstantBuffer
	{
		constexpr static u32 MAX_NAME_LENGTH = 64;
		u8* BackingMemory;
		gfx::ConstantBuffer GfxState;
		char Name[MAX_NAME_LENGTH];
		u32 Slot;
		u32 Size;
	};
	struct SetConstant
	{
		const char* VariableName;
		ast::Expression Value;
		ConstantBuffer* CB;
		u32 Offset;
		u32 Size;
		VariableType Type;
	};
	struct Dispatch
	{
		ComputeShader* Shader;
		bool ThreadPerPixel;
		ast::Expression Groups;
		Buffer* IndirectArgs;
		u32 IndirectArgsOffset;
		Array<Bind> Binds;
		Array<SetConstant> Constants;
		Array<ConstantBuffer> CBs;
		gfx::DispatchData GfxState;
	};
	struct Draw
	{
		Topology Topology;
		RasterizerState* RState;
		DepthStencilState* DSState;
		VertexShader* VShader;
		PixelShader* PShader;
		Array<Buffer*> VertexBuffers;
		Buffer* IndexBuffer;
		Buffer* InstancedIndirectArgs;
		Buffer* IndexedInstancedIndirectArgs;
		u32 IndirectArgsOffset;
		u32 VertexCount;
		u32 InstanceCount;
		u8 StencilRef;
		Array<View*> RenderTargets;
		View* DepthStencil;
		Array<Viewport*> Viewports;
		Array<BlendState*> BlendStates;
		Array<Bind> VSBinds;
		Array<Bind> PSBinds;
		Array<SetConstant> VSConstants;
		Array<SetConstant> PSConstants;
		Array<ConstantBuffer> VSCBs;
		Array<ConstantBuffer> PSCBs;
		gfx::BlendState BlendGfxState;
		gfx::DrawData GfxState;
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
	struct Resolve
	{
		Texture* Src;
		Texture* Dst;
	};
	struct ObjDraw
	{
		Array<Draw*> PerMeshDraws;
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
			Resolve* Resolve;
			ObjDraw* ObjDraw;
		};
	};
	struct Constant
	{
		const char* Name;
		ast::Expression Expr;
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
		Array<Pass> Passes;
		Array<Dispatch*> Dispatches;
		Array<Draw*> Draws;
		Array<ObjDraw*> ObjDraws;
		Array<ComputeShader*> CShaders;
		Array<VertexShader*> VShaders;
		Array<PixelShader*> PShaders;
		Array<SizeOfRequest> SizeOfRequests;
		Array<Buffer*> Buffers;
		Array<Texture*> Textures;
		Array<Sampler*> Samplers;
		Array<View*> Views;
		Array<RasterizerState*> RasterizerStates;
		Array<DepthStencilState*> DepthStencilStates;
		Array<Constant*> Constants;
		Array<Tuneable*> Tuneables;
		Array<Texture*> Outputs;

		// TODO: Move D3D data into separate struct
		Array<gfx::ShaderResourceView> OutputViews;

		alloc::LinAlloc Alloc;
	};
}
