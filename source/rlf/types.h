
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
	VariableType Bool2Type = { VariableFormat::Bool, 2 };
	VariableType Bool3Type = { VariableFormat::Bool, 3 };
	VariableType Bool4Type = { VariableFormat::Bool, 4 };
	VariableType IntType = { VariableFormat::Int, 1 };
	VariableType Int2Type = { VariableFormat::Int, 2 };
	VariableType Int3Type = { VariableFormat::Int, 3 };
	VariableType Int4Type = { VariableFormat::Int, 4 };
	VariableType UintType = { VariableFormat::Uint, 1 };
	VariableType Uint2Type = { VariableFormat::Uint, 2 };
	VariableType Uint3Type = { VariableFormat::Uint, 3 };
	VariableType Uint4Type = { VariableFormat::Uint, 4 };
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
			bool4 Bool4Val;
			i32 IntVal;
			int4 Int4Val;
			u32 UintVal;
			uint2 Uint2Val;
			uint3 Uint3Val;
			uint4 Uint4Val;
			float FloatVal;
			float2 Float2Val;
			float3 Float3Val;
			float4 Float4Val;
			float4x4 Float4x4Val;
		};
	};

	const char* TypeFmtToString(VariableFormat fmt)
	{
		const char* names[] = {
			"Bool",
			"Int",
			"Uint",
			"Float",
			"Float4x4",
		};
		return names[(u32)fmt];
	}

	template <typename T>
	struct Array
	{
		u32 Count;
		T* Data;

		T& operator[](size_t index) { Assert(index < Count, "invalid"); return Data[index]; }
		T* begin() { return Data; }
		T* end() { return Data+Count; }
	};
}
