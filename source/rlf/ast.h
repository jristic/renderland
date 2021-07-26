
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
	ErrorInfo Info;
};

struct Node 
{
	enum class Special {
		None,
		Operator,
	} Spec;
	const char* Location;
	Node() : Spec(Special::None) {}
	virtual ~Node() {}
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const = 0;
};

void Evaluate(const EvaluationContext& ec, const Node* ast, Result& res, EvaluateErrorState& es);

void Convert(Result& res, VariableFormat fmt);



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

struct Group : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	Node* Sub;
};

struct BinaryOp : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	enum class Type {
		Add, Subtract, Multiply, Divide
	};
	std::vector<Node*> Args;
	std::vector<Type> Ops;
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
