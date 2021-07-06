
namespace rlf {
namespace ast {


void EvaluateFloat3(const EvaluationContext& ec, std::vector<Node*> args, Result& res);
void EvaluateFloat2(const EvaluationContext& ec, std::vector<Node*> args, Result& res);
void EvaluateTime(const EvaluationContext& ec, std::vector<Node*> args, Result& res);
void EvaluateDisplaySize(const EvaluationContext& ec, std::vector<Node*> args, Result& res);
void EvaluateLookAt(const EvaluationContext& ec, std::vector<Node*> args, Result& res);
void EvaluateProjection(const EvaluationContext& ec, std::vector<Node*> args, Result& res);

u32 LowerHash(const char* str)
{
	unsigned long h = 5381;
	unsigned const char* us = (unsigned const char *) str;
	while(*us != '\0') {
		h = ((h << 5) + h) + tolower(*us);
		us++;
	}
	return h; 
}

typedef void (*FunctionEvaluate)(const EvaluationContext&, std::vector<Node*>, Result&);
std::unordered_map<u32, FunctionEvaluate> FuncMap = {
	{ LowerHash("Float2"), EvaluateFloat2 },
	{ LowerHash("Float3"), EvaluateFloat3 },
	{ LowerHash("Time"), EvaluateTime },
	{ LowerHash("DisplaySize"), EvaluateDisplaySize },
	{ LowerHash("LookAt"), EvaluateLookAt },
	{ LowerHash("Projection"), EvaluateProjection },
};

struct AstException
{
	std::string Message;
};

void Evaluate(const EvaluationContext& ec, const Node* ast, Result& res, EvaluateErrorState& es)
{
	es.EvaluateSuccess = true;
	try {
		ast->Evaluate(ec, res);
	}
	catch (AstException ae)
	{
		es.EvaluateSuccess = false;
		es.ErrorMessage = ae.Message;
	}
}

void AstError(const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	AstException ae;
	ae.Message = buf;
	throw ae;
}

#define AstAssert(expression, message, ...) 	\
do {											\
	if (!(expression)) {						\
		AstError(message, ##__VA_ARGS__);	\
	}											\
} while (0);									\

const char* TypeToString(ResultType type)
{
	const char* names[] = {
		"Float",
		"Float2",
		"Float3",
		"Float4",
		"Float4x4",
	};
	return names[(u32)type];
}

void Expect(ResultType type, Result& res, const char* name)
{
	AstAssert(res.Type == type, "Expected %s to be type %s but got %s.",
		name, TypeToString(type), TypeToString(res.Type));
}

void ExpandFloat4(Result& res)
{
	switch (res.Type)
	{
	case ResultType::Float:
		res.Float4Val.y = res.Float4Val.z = res.Float4Val.w = res.Float4Val.x;
		break;
	case ResultType::Float2:
		res.Float4Val.z = res.Float4Val.w = res.Float4Val.x;
		break;
	case ResultType::Float3:
		res.Float4Val.w = res.Float4Val.x;
		break;
	case ResultType::Float4:
		break;
	default:
		Unimplemented();
	}
}

// -----------------------------------------------------------------------------
// ------------------------------ NODE EVALS -----------------------------------
// -----------------------------------------------------------------------------
void FloatLiteral::Evaluate(const EvaluationContext&, Result& res) const
{
	res.Type = ResultType::Float;
	res.FloatVal = Val;
}

void Subscript::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result subjectRes;
	Subject->Evaluate(ec, subjectRes);
	AstAssert(subjectRes.Type != ResultType::Bool, "Subscripts aren't usable on bool types");
	AstAssert((Index + 1) * 4 <= subjectRes.Size(), "Invalid subscript for this type");
	AstAssert(subjectRes.Type != ResultType::Float4x4,
		"Subscripts aren't usable on Matrix types");
	res.Type = ResultType::Float;
	if (Index == 0)
		res.FloatVal = subjectRes.Float4Val.x;
	else if (Index == 1)
		res.FloatVal = subjectRes.Float4Val.y;
	else if (Index == 2)
		res.FloatVal = subjectRes.Float4Val.z;
	else if (Index == 3)
		res.FloatVal = subjectRes.Float4Val.w;
	else
		Unimplemented();
}

void Multiply::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	AstAssert(arg1Res.Type != ResultType::Bool && arg2Res.Type != ResultType::Bool, 
		"Bool multiplication not supported.");
	if (arg1Res.Type == ResultType::Float4x4 ||
		arg2Res.Type == ResultType::Float4x4)
	{
		Expect(ResultType::Float4x4, arg1Res, "lhs");
		Expect(ResultType::Float4x4, arg2Res, "rhs");
		AstAssert(arg1Res.Type == ResultType::Float4x4 &&
			arg2Res.Type == ResultType::Float4x4, 
			"Matrix types can only be multiplied with other Matrix types");
		res.Type = ResultType::Float4x4;
		res.Float4x4Val = arg1Res.Float4x4Val * arg2Res.Float4x4Val;
		return;
	}
	AstAssert(arg1Res.Type == arg2Res.Type || arg1Res.Type == ResultType::Float ||
		arg2Res.Type == ResultType::Float, "Vector size mismatch in multiply, %s and %s",
		TypeToString(arg1Res.Type), TypeToString(arg2Res.Type));
	ExpandFloat4(arg1Res);
	ExpandFloat4(arg2Res);
	res.Type = arg1Res.Type == ResultType::Float ? arg2Res.Type : arg1Res.Type;
	res.Float4Val = arg1Res.Float4Val * arg2Res.Float4Val;
}

void Divide::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	AstAssert(arg1Res.Type != ResultType::Bool && arg2Res.Type != ResultType::Bool, 
		"Bool division not supported.");
	AstAssert(arg1Res.Type != ResultType::Float4x4 && 
		arg2Res.Type != ResultType::Float4x4,
		"Matrix types not supported in divides.");
	AstAssert(arg1Res.Type == arg2Res.Type || arg1Res.Type == ResultType::Float ||
		arg2Res.Type == ResultType::Float, "Vector size mismatch in multiply, %s and %s",
		TypeToString(arg1Res.Type), TypeToString(arg2Res.Type));
	ExpandFloat4(arg1Res);
	ExpandFloat4(arg2Res);
	res.Type = arg1Res.Type == ResultType::Float ? arg2Res.Type : arg1Res.Type;
	res.Float4Val = arg1Res.Float4Val / arg2Res.Float4Val;
}

void TuneableRef::Evaluate(const EvaluationContext&, Result& res) const
{
	switch (Tune->T)
	{
	case Tuneable::Type::Bool:
	{
		res.Type = ResultType::Bool;
		res.BoolVal = Tune->BoolVal;
		break;
	}
	case Tuneable::Type::Float:
	{
		res.Type = ResultType::Float;
		res.FloatVal = Tune->FloatVal;
		break;
	}
	default:
		Unimplemented();
	}
}

void Function::Evaluate(const EvaluationContext& ec, Result& res) const
{
	u32 funcHash = LowerHash(Name.c_str());
	AstAssert(FuncMap.count(funcHash) == 1, "No function named %s exists.", Name.c_str());
	FunctionEvaluate fi = FuncMap[funcHash];
	fi(ec, Args, res);
}


// -----------------------------------------------------------------------------
// ------------------------------ FUNCTION EVALS -------------------------------
// -----------------------------------------------------------------------------
void EvaluateFloat3(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 3, "Float3 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
	Expect(ResultType::Float, resX, "arg1");
	Expect(ResultType::Float, resY, "arg2");
	Expect(ResultType::Float, resZ, "arg3");
	res.Type = ResultType::Float3;
	res.Float3Val.x = resX.FloatVal;
	res.Float3Val.y = resY.FloatVal;
	res.Float3Val.z = resZ.FloatVal;
}

void EvaluateFloat2(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 2, "Float2 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	Expect(ResultType::Float, resX, "arg1");
	Expect(ResultType::Float, resY, "arg2");
	res.Type = ResultType::Float2;
	res.Float3Val.x = resX.FloatVal;
	res.Float3Val.y = resY.FloatVal;
}

void EvaluateTime(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 0, "Time does not take a param");
	res.Type = ResultType::Float;
	res.FloatVal = ec.Time;
}

void EvaluateDisplaySize(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 0, "DisplaySize does not take a param");
	res.Type = ResultType::Float2;
	res.Float2Val.x = (float)ec.DisplaySize.x;
	res.Float2Val.y = (float)ec.DisplaySize.y;
}

void EvaluateLookAt(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 2, "LookAt takes 2 params");
	Result fromRes, toRes;
	args[0]->Evaluate(ec, fromRes);
	args[1]->Evaluate(ec, toRes);
	Expect(ResultType::Float3, fromRes, "arg1 (from)");
	Expect(ResultType::Float3, toRes, "arg2 (to)");
	res.Type = ResultType::Float4x4;
	res.Float4x4Val = lookAt(fromRes.Float3Val, toRes.Float3Val);
}

void EvaluateProjection(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 4, "Projection takes 4 params");
	Result fovRes, aspectRes, nearRes, farRes;
	args[0]->Evaluate(ec, fovRes);
	args[1]->Evaluate(ec, aspectRes);
	args[2]->Evaluate(ec, nearRes);
	args[3]->Evaluate(ec, farRes);
	Expect(ResultType::Float, fovRes, "arg1 (fov)");
	Expect(ResultType::Float, aspectRes, "arg2 (aspect)");
	Expect(ResultType::Float, nearRes, "arg3 (znear)");
	Expect(ResultType::Float, farRes, "arg4 (zfar)");
	res.Type = ResultType::Float4x4;
	res.Float4x4Val = projection(fovRes.FloatVal, aspectRes.FloatVal, 
		nearRes.FloatVal, farRes.FloatVal);
}

#undef AstAssert


} // namespace ast
} // namespace rlf
