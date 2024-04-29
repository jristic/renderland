
namespace rlf {
namespace ast {


struct EvaluationContext
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

#define NODE_TYPE_TUPLE \
	NODE_TYPE_ENTRY(UintLiteral,		 UintLiteral_Evaluate, 		None_GetDependency) \
	NODE_TYPE_ENTRY(IntLiteral,			 IntLiteral_Evaluate, 		None_GetDependency) \
	NODE_TYPE_ENTRY(FloatLiteral,		 FloatLiteral_Evaluate, 	None_GetDependency) \
	NODE_TYPE_ENTRY(Subscript,			 Subscript_Evaluate, 		Subscript_GetDependency) \
	NODE_TYPE_ENTRY(Group,				 Group_Evaluate, 			Group_GetDependency) \
	NODE_TYPE_ENTRY(BinaryOp,			 BinaryOp_Evaluate, 		BinaryOp_GetDependency) \
	NODE_TYPE_ENTRY(Join,				 Join_Evaluate, 			Join_GetDependency) \
	NODE_TYPE_ENTRY(VariableRef,		 VariableRef_Evaluate, 		VariableRef_GetDependency) \
	NODE_TYPE_ENTRY(Function,			 Function_Evaluate, 		Function_GetDependency) \
	NODE_TYPE_ENTRY(SizeOf,				 SizeOf_Evaluate, 			None_GetDependency) \

#define NODE_TYPE_ENTRY(type, eval_func, dep_func) type,
enum class NodeType
{
	NODE_TYPE_TUPLE
	Count
};
#undef NODE_TYPE_ENTRY

struct Node
{
	const char* Location;
	NodeType Type;
};

struct Expression 
{
	const Node* TopNode;

	DependencyInfo Dep;
	Result CachedResult;
	bool CacheValid;

	bool IsValid() const { return TopNode != 0; }
	bool VariesByTime() const { return Dep.VariesByFlags & VariesBy_Time; }
	bool Constant() const { return Dep.VariesByFlags == VariesBy_None; }
};

void Evaluate(const EvaluationContext& ec, Expression& expr, Result& res, 
	ErrorState& es);
void GetDependency(const Node* node, DependencyInfo& dep);

void Convert(Result& res, VariableFormat fmt);


struct UintLiteral
{
	static constexpr NodeType NodeType = NodeType::UintLiteral;
	Node Common;
	u32 Val;
};

struct IntLiteral
{
	static constexpr NodeType NodeType = NodeType::IntLiteral;
	Node Common;
	i32 Val;
};

struct FloatLiteral
{
	static constexpr NodeType NodeType = NodeType::FloatLiteral;
	Node Common;
	float Val;
};

struct Subscript
{
	static constexpr NodeType NodeType = NodeType::Subscript;
	Node Common;
	Node* Subject;
	u8 Index[4];
};

struct Group
{
	static constexpr NodeType NodeType = NodeType::Group;
	Node Common;
	Node* Sub;
};

struct BinaryOp
{
	static constexpr NodeType NodeType = NodeType::BinaryOp;
	Node Common;
	enum class Type {
		Add, Subtract, Multiply, Divide
	};
	Node* LArg;
	Node* RArg;
	Type Op;
};

struct Join
{
	static constexpr NodeType NodeType = NodeType::Join;
	Node Common;
	Array<Node*> Comps;
};

struct VariableRef
{
	static constexpr NodeType NodeType = NodeType::VariableRef;
	Node Common;
	bool IsTuneable;
	void* M;
};

struct Function
{
	static constexpr NodeType NodeType = NodeType::Function;
	Node Common;
	const char* Name;
	Array<Node*> Args;
};

struct SizeOf
{
	static constexpr NodeType NodeType = NodeType::SizeOf;
	Node Common;
	const char* StructName;
	u32 Size;
};


}
}
