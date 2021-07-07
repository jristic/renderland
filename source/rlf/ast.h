
namespace rlf { struct Tuneable;
namespace ast {


struct EvaluationContext // TODO: collapse with executecontext?
{
	uint2 DisplaySize;
	float Time;
};

struct Result
{
	VariableType Type;
	Variable Value;
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

struct TuneableRef : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	Tuneable* Tune;
};

struct Function : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	std::string Name;
	std::vector<Node*> Args;
};


}
}
