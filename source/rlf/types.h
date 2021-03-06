
namespace rlf
{

	enum class VariableFormat
	{
		Bool,
		Int,
		Uint,
		Float,
		Float4x4,
	};

	struct VariableType {
		VariableFormat Fmt;
		u32 Dim;
	};

	bool operator==(VariableType lhs, VariableType rhs)
	{
		return lhs.Fmt == rhs.Fmt && lhs.Dim == rhs.Dim;
	}
	bool operator!=(VariableType lhs, VariableType rhs)
	{
		return lhs.Fmt != rhs.Fmt || lhs.Dim != rhs.Dim;
	}

	VariableType BoolType = { VariableFormat::Bool, 1 };
	VariableType IntType = { VariableFormat::Int, 1 };
	VariableType UintType = { VariableFormat::Uint, 1 };
	VariableType Uint2Type = { VariableFormat::Uint, 2 };
	VariableType FloatType = { VariableFormat::Float, 1 };
	VariableType Float2Type = { VariableFormat::Float, 2 };
	VariableType Float3Type = { VariableFormat::Float, 3 };
	VariableType Float4Type = { VariableFormat::Float, 4 };
	VariableType Float4x4Type = { VariableFormat::Float4x4, 16 };

	struct Variable
	{
		Variable() {}
		union {
			bool BoolVal;
			i32 IntVal;
			int4 Int4Val;
			u32 UintVal;
			uint4 Uint4Val;
			float FloatVal;
			float2 Float2Val;
			float3 Float3Val;
			float4 Float4Val;
			float4x4 Float4x4Val;
		};
	};

	const char* TypeToString(VariableType type)
	{
		const char* names[] = {
			"Bool",
			"Int",
			"Uint",
			"Float",
			"Float4x4",
		};
		return names[(u32)type.Fmt];
	}
}
