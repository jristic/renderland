
namespace rlf {
namespace ast {


void EvaluateFloat3(const EvaluationContext& ec, std::vector<Node*> args, Result& res);
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
	{ LowerHash("Float3"), EvaluateFloat3 },
	{ LowerHash("Time"), EvaluateTime },
	{ LowerHash("DisplaySize"), EvaluateDisplaySize },
	{ LowerHash("LookAt"), EvaluateLookAt },
	{ LowerHash("Projection"), EvaluateProjection },
};


// -----------------------------------------------------------------------------
// ------------------------------ NODE EVALS -----------------------------------
// -----------------------------------------------------------------------------
void FloatLiteral::Evaluate(const EvaluationContext&, Result& res)
{
	res.Type = Result::Type::Float;
	res.FloatVal = Val;
}

void Subscript::Evaluate(const EvaluationContext& ec, Result& res)
{
	Result subjectRes;
	Subject->Evaluate(ec, subjectRes);
	Assert(subjectRes.Type == Result::Type::Float2, "not handled"); // TODO: expand
	res.Type = Result::Type::Float;
	if (Index == 0)
		res.FloatVal = subjectRes.Float2Val.x;
	else if (Index == 1)
		res.FloatVal = subjectRes.Float2Val.y;
	else
		Unimplemented();
}

void Multiply::Evaluate(const EvaluationContext& ec, Result& res)
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	Assert(arg1Res.Type == Result::Type::Float4x4 && 
		arg2Res.Type == Result::Type::Float4x4, "invalid");
	res.Type = Result::Type::Float4x4;
	res.Float4x4Val = arg1Res.Float4x4Val * arg2Res.Float4x4Val;
}

void Divide::Evaluate(const EvaluationContext& ec, Result& res)
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	Assert(arg1Res.Type == Result::Type::Float && arg2Res.Type == Result::Type::Float,
		"invalid");
	res.Type = Result::Type::Float;
	res.FloatVal = arg1Res.FloatVal / arg2Res.FloatVal;
}

void FunctionNode::Evaluate(const EvaluationContext& ec, Result& res)
{
	u32 funcHash = LowerHash(Name.c_str());
	Assert(FuncMap.count(funcHash) == 1, "No function named %s exists.", Name.c_str());
	FunctionEvaluate fi = FuncMap[funcHash];
	fi(ec, Args, res);
}


// -----------------------------------------------------------------------------
// ------------------------------ FUNCTION EVALS -------------------------------
// -----------------------------------------------------------------------------
void EvaluateFloat3(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	Assert(args.size() == 3, "Float3 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
	Assert(resX.Type == Result::Type::Float && resY.Type == Result::Type::Float && 
		resZ.Type == Result::Type::Float, "invalid");
	res.Type = Result::Type::Float3;
	res.Float3Val.x = resX.FloatVal;
	res.Float3Val.y = resY.FloatVal;
	res.Float3Val.z = resZ.FloatVal;
}

void EvaluateTime(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	Assert(args.size() == 0, "Time does not take a param");
	res.Type = Result::Type::Float;
	res.FloatVal = ec.Time;
}

void EvaluateDisplaySize(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	Assert(args.size() == 0, "DisplaySize does not take a param");
	res.Type = Result::Type::Float2;
	res.Float2Val.x = (float)ec.DisplaySize.x;
	res.Float2Val.y = (float)ec.DisplaySize.y;
}

void EvaluateLookAt(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	Assert(args.size() == 2, "LookAt takes 2 params");
	Result fromRes, toRes;
	args[0]->Evaluate(ec, fromRes);
	args[1]->Evaluate(ec, toRes);
	Assert(fromRes.Type == Result::Type::Float3 && toRes.Type == Result::Type::Float3,
		"invalid");
	res.Type = Result::Type::Float4x4;
	res.Float4x4Val = lookAt(fromRes.Float3Val, toRes.Float3Val);
}

void EvaluateProjection(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	Assert(args.size() == 4, "Projection takes 4 params");
	Result fovRes, aspectRes, nearRes, farRes;
	args[0]->Evaluate(ec, fovRes);
	args[1]->Evaluate(ec, aspectRes);
	args[2]->Evaluate(ec, nearRes);
	args[3]->Evaluate(ec, farRes);
	Assert(fovRes.Type == Result::Type::Float && aspectRes.Type == Result::Type::Float &&
		nearRes.Type == Result::Type::Float && farRes.Type == Result::Type::Float,
		"invalid");
	res.Type = Result::Type::Float4x4;
	res.Float4x4Val = projection(fovRes.FloatVal, aspectRes.FloatVal, 
		nearRes.FloatVal, farRes.FloatVal);
}



}
}
