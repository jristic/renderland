
namespace rlf
{

	enum class VariableType
	{
		Bool,
		Int,
		Uint,
		Float,
		Float2,
		Float3,
		Float4,
		Float4x4,
	};

	struct Variable
	{
		Variable() {}
		union {
			bool BoolVal;
			i32 IntVal;
			u32 UintVal;
			float FloatVal;
			float2 Float2Val;
			float3 Float3Val;
			float4 Float4Val;
			float4x4 Float4x4Val;
		};
	};
	


	u32 TypeToSize(VariableType type) {
		u32 sizes[] = {
			4, 4, 4, 4, 8, 12, 16, 64
		};
		return sizes[(u32)type];
	}
	const char* TypeToString(VariableType type)
	{
		const char* names[] = {
			"Bool",
			"Int",
			"Uint",
			"Float",
			"Float2",
			"Float3",
			"Float4",
			"Float4x4",
		};
		return names[(u32)type];
	}
}
