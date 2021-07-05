
namespace rlf {
namespace ast {


struct EvaluationContext // TODO: collapse with executecontext?
{
	uint2 DisplaySize;
	float Time;
};

struct Result
{
	Result() {}
	enum class Type
	{
		Float,
		Float2,
		Float3,
		Float4,
		Float4x4,
	};
	Type Type;
	union {
		float FloatVal;
		float2 Float2Val;
		float3 Float3Val;
		float4 Float4Val;
		float4x4 Float4x4Val;
	};
	u32 Size() {
		u32 sizes[] = {
			4, 8, 12, 16, 64
		};
		return sizes[(u32)Type];
	}
};

struct EvaluateErrorState
{
	bool EvaluateSuccess;
	std::string ErrorMessage;
};

struct Node 
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const = 0;
	virtual ~Node() {}
};

void Evaluate(const EvaluationContext& ec, const Node* ast, Result& res, EvaluateErrorState& es);



struct FloatLiteral : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	float Val;
};

struct Subscript : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	Node* Subject;
	u32 Index;
};

struct Multiply : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	Node* Arg1;
	Node* Arg2;
};

struct Divide : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	Node* Arg1;
	Node* Arg2;
};

struct Function : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	std::string Name;
	std::vector<Node*> Args;
};


}
}
