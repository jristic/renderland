
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
		Float4x4,
	};
	Type Type;
	union {
		float FloatVal;
		float2 Float2Val;
		float3 Float3Val;
		float4x4 Float4x4Val;
	};
	u32 Size() {
		u32 sizes[] = {
			4, 8, 12, 64
		};
		return sizes[(u32)Type];
	}
};

struct Node 
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) = 0;
	virtual ~Node() {}
};

struct FloatLiteral : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	float Val;
};

struct Float3 : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	Node* X;
	Node* Y;
	Node* Z;
};

struct Subscript : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	Node* Subject;
	u32 Index;
};

struct Multiply : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	Node* Arg1;
	Node* Arg2;
};

struct Divide : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	Node* Arg1;
	Node* Arg2;
};

struct Time : Node 
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
};

struct DisplaySize : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
};

struct LookAt : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	Node* From;
	Node* To;
};

struct Projection : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result *res) override;
	Node* Fov;
	Node* Aspect;
	Node* ZNear;
	Node* ZFar;
};


}
}
