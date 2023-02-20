
namespace rlf {
namespace ast {


struct EvaluationContext // TODO: collapse with executecontext?
{
	uint2 DisplaySize;
	float Time;
	u32 ChangedThisFrameFlags;
};

struct Result
{
	VariableType Type;
	Variable Value;
};

enum VariesBy
{
	VariesBy_None =			0,
	VariesBy_DisplaySize = 	1,
	VariesBy_Tuneable =		2,
	VariesBy_Time =			4,
};
struct DependencyInfo
{
	u32 VariesByFlags = VariesBy_None;
};

struct Node 
{
	enum class Special {
		None,
		Operator,
	} Spec;
	const char* Location;
	DependencyInfo Dep;
	Result CachedResult;
	bool CacheValid = false;

	Node() : Spec(Special::None) {}
	virtual ~Node() {}

	virtual void Evaluate(const EvaluationContext& ec, Result& res) const = 0;
	virtual void GetDependency(DependencyInfo& dep) const = 0;

	bool Constant() const { return Dep.VariesByFlags == VariesBy_None; }
	bool VariesByDisplaySize() const { return Dep.VariesByFlags & VariesBy_DisplaySize; }
	bool VariesByTuneable() const { return Dep.VariesByFlags & VariesBy_Tuneable; }
	bool VariesByTime() const { return Dep.VariesByFlags & VariesBy_Time; }
};

void Evaluate(const EvaluationContext& ec, Node* ast, Result& res, ErrorState& es);

void Convert(Result& res, VariableFormat fmt);



struct UintLiteral : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	u32 Val;
};

struct IntLiteral : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	i32 Val;
};

struct FloatLiteral : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	float Val;
};

struct Subscript : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	Node* Subject;
	i8 Index[4] = {-1,-1,-1,-1};
};

struct Group : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	Node* Sub;
};

struct BinaryOp : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	enum class Type {
		Add, Subtract, Multiply, Divide
	};
	std::vector<Node*> Args;
	std::vector<Type> Ops;
};

struct Join : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	std::vector<Node*> Comps;
};

struct VariableRef : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	bool isTuneable;
	void* M;
};

struct Function : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	std::string Name;
	std::vector<Node*> Args;
};

struct SizeOf : Node
{
	virtual void Evaluate(const EvaluationContext& ec, Result& res) const override;
	virtual void GetDependency(DependencyInfo& dep) const override;
	std::string StructName;
	u32 Size;
};


}
}
