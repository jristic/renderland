
namespace rlf {
namespace ast {


struct AstException
{
	ErrorInfo Info;
};

void Evaluate(const EvaluationContext& ec, const Node* ast, Result& res, 
	EvaluateErrorState& es)
{
	es.EvaluateSuccess = true;
	try {
		ast->Evaluate(ec, res);
	}
	catch (AstException ae)
	{
		es.EvaluateSuccess = false;
		es.Info = ae.Info;
	}
}

void AstError(const Node* n, const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	AstException ae;
	ae.Info.Location = n->Location;
	ae.Info.Message = buf;
	throw ae;
}

#define AstAssert(node, expression, message, ...) 	\
do {												\
	if (!(expression)) {							\
		AstError(node, message, ##__VA_ARGS__);		\
	}												\
} while (0);										\



void Expect(const Node* n, VariableType type, Result& res, const char* name)
{
	AstAssert(n, res.Type == type, "Expected %s to be type %s but got %s.",
		name, TypeToString(type), TypeToString(res.Type));
}

VariableType DetermineResultType(const Node* n, VariableType t1, VariableType t2)
{
	AstAssert(n, t1.Dim == t2.Dim || t1.Dim == 1 || t2.Dim == 1, 
		"Vector size mismatch in operation, %u vs. %u", t1.Dim, t2.Dim);
	VariableType t;
	t.Dim = t1.Dim == 1 ? t2.Dim : t1.Dim;
	if (t1.Fmt == VariableFormat::Float || t2.Fmt == VariableFormat::Float)
		t.Fmt = VariableFormat::Float;
	else if (t1.Fmt == VariableFormat::Int || t2.Fmt == VariableFormat::Int)
		t.Fmt = VariableFormat::Int;
	else if (t1.Fmt == VariableFormat::Uint || t2.Fmt == VariableFormat::Uint)
		t.Fmt = VariableFormat::Uint;
	else
		t.Fmt = VariableFormat::Bool;
	return t;
}

void Expand(Result& res)
{
	Assert(res.Type.Fmt != VariableFormat::Float4x4, "Invalid type in expand");
	if (res.Type.Dim == 1)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Bool:
			res.Value.Bool4Val.w = res.Value.Bool4Val.z = res.Value.Bool4Val.y = 
				res.Value.Bool4Val.x;
			break;
		case VariableFormat::Int:
			res.Value.Int4Val.w = res.Value.Int4Val.z = res.Value.Int4Val.y = 
				res.Value.Int4Val.x;
			break;
		case VariableFormat::Uint:
			res.Value.Uint4Val.w = res.Value.Uint4Val.z = res.Value.Uint4Val.y = 
				res.Value.Uint4Val.x;
			break;
		case VariableFormat::Float:
			res.Value.Float4Val.w = res.Value.Float4Val.z = res.Value.Float4Val.y = 
				res.Value.Float4Val.x;
			break;
		default:
			Unimplemented();
		}
	}
}

void Convert(Result& res, VariableFormat fmt)
{
	if (res.Type.Fmt == fmt)
		return;
	if (fmt == VariableFormat::Bool)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Int:
			res.Value.Bool4Val.x = res.Value.Bool4Val.x ? 1 : 0;
			res.Value.Bool4Val.y = res.Value.Bool4Val.y ? 1 : 0;
			res.Value.Bool4Val.z = res.Value.Bool4Val.z ? 1 : 0;
			res.Value.Bool4Val.w = res.Value.Bool4Val.w ? 1 : 0;
			break;
		case VariableFormat::Uint:
			res.Value.Bool4Val.x = (i32)res.Value.Uint4Val.x;
			res.Value.Bool4Val.y = (i32)res.Value.Uint4Val.y;
			res.Value.Bool4Val.z = (i32)res.Value.Uint4Val.z;
			res.Value.Bool4Val.w = (i32)res.Value.Uint4Val.w;
			break;
		case VariableFormat::Float:
			res.Value.Bool4Val.x = (i32)res.Value.Float4Val.x;
			res.Value.Bool4Val.y = (i32)res.Value.Float4Val.y;
			res.Value.Bool4Val.z = (i32)res.Value.Float4Val.z;
			res.Value.Bool4Val.w = (i32)res.Value.Float4Val.w;
			break;
		default:
			Unimplemented();
		}
	}
	else if (fmt == VariableFormat::Int)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Bool:
			res.Value.Int4Val.x = res.Value.Bool4Val.x ? 1 : 0;
			res.Value.Int4Val.y = res.Value.Bool4Val.y ? 1 : 0;
			res.Value.Int4Val.z = res.Value.Bool4Val.z ? 1 : 0;
			res.Value.Int4Val.w = res.Value.Bool4Val.w ? 1 : 0;
			break;
		case VariableFormat::Uint:
			res.Value.Int4Val.x = (i32)res.Value.Uint4Val.x;
			res.Value.Int4Val.y = (i32)res.Value.Uint4Val.y;
			res.Value.Int4Val.z = (i32)res.Value.Uint4Val.z;
			res.Value.Int4Val.w = (i32)res.Value.Uint4Val.w;
			break;
		case VariableFormat::Float:
			res.Value.Int4Val.x = (i32)res.Value.Float4Val.x;
			res.Value.Int4Val.y = (i32)res.Value.Float4Val.y;
			res.Value.Int4Val.z = (i32)res.Value.Float4Val.z;
			res.Value.Int4Val.w = (i32)res.Value.Float4Val.w;
			break;
		default:
			Unimplemented();
		}
	}
	else if (fmt == VariableFormat::Uint)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Bool:
			res.Value.Uint4Val.x = res.Value.Bool4Val.x ? 1 : 0;
			res.Value.Uint4Val.y = res.Value.Bool4Val.y ? 1 : 0;
			res.Value.Uint4Val.z = res.Value.Bool4Val.z ? 1 : 0;
			res.Value.Uint4Val.w = res.Value.Bool4Val.w ? 1 : 0;
			break;
		case VariableFormat::Int:
			res.Value.Uint4Val.x = (u32)res.Value.Int4Val.x;
			res.Value.Uint4Val.y = (u32)res.Value.Int4Val.y;
			res.Value.Uint4Val.z = (u32)res.Value.Int4Val.z;
			res.Value.Uint4Val.w = (u32)res.Value.Int4Val.w;
			break;
		case VariableFormat::Float:
			res.Value.Uint4Val.x = (u32)res.Value.Float4Val.x;
			res.Value.Uint4Val.y = (u32)res.Value.Float4Val.y;
			res.Value.Uint4Val.z = (u32)res.Value.Float4Val.z;
			res.Value.Uint4Val.w = (u32)res.Value.Float4Val.w;
			break;
		default:
			Unimplemented();
		}
	}
	else if (fmt == VariableFormat::Float)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Bool:
			res.Value.Float4Val.x = res.Value.Bool4Val.x ? 1.f : 0.f;
			res.Value.Float4Val.y = res.Value.Bool4Val.y ? 1.f : 0.f;
			res.Value.Float4Val.z = res.Value.Bool4Val.z ? 1.f : 0.f;
			res.Value.Float4Val.w = res.Value.Bool4Val.w ? 1.f : 0.f;
			break;
		case VariableFormat::Int:
			res.Value.Float4Val.x = (float)res.Value.Int4Val.x;
			res.Value.Float4Val.y = (float)res.Value.Int4Val.y;
			res.Value.Float4Val.z = (float)res.Value.Int4Val.z;
			res.Value.Float4Val.w = (float)res.Value.Int4Val.w;
			break;
		case VariableFormat::Uint:
			res.Value.Float4Val.x = (float)res.Value.Uint4Val.x;
			res.Value.Float4Val.y = (float)res.Value.Uint4Val.y;
			res.Value.Float4Val.z = (float)res.Value.Uint4Val.z;
			res.Value.Float4Val.w = (float)res.Value.Uint4Val.w;
			break;
		default:
			Unimplemented();
		}
	}
}

void OperatorAdd(const Node* n, const Result& arg1, const Result& arg2, Result& res)
{
	switch (res.Type.Fmt)
	{
	case VariableFormat::Float:
		res.Value.Float4Val = arg1.Value.Float4Val + arg2.Value.Float4Val;
		break;
	case VariableFormat::Int:
		res.Value.Int4Val = arg1.Value.Int4Val + arg2.Value.Int4Val;
		break;
	case VariableFormat::Uint:
		res.Value.Uint4Val = arg1.Value.Uint4Val + arg2.Value.Uint4Val;
		break;
	case VariableFormat::Bool:
		AstError(n, "Bool add has not been defined");
		break;
	default:
		Unimplemented();
	}
}

void OperatorSubtract(const Node* n, const Result& arg1, const Result& arg2, Result& res)
{
	switch (res.Type.Fmt)
	{
	case VariableFormat::Float:
		res.Value.Float4Val = arg1.Value.Float4Val - arg2.Value.Float4Val;
		break;
	case VariableFormat::Int:
		res.Value.Int4Val = arg1.Value.Int4Val - arg2.Value.Int4Val;
		break;
	case VariableFormat::Uint:
		res.Value.Uint4Val = arg1.Value.Uint4Val - arg2.Value.Uint4Val;
		break;
	case VariableFormat::Bool:
		AstError(n, "Bool subtract has not been defined");
		break;
	default:
		Unimplemented();
	}
}

void OperatorMultiply(const Node* n, const Result& arg1, const Result& arg2, Result& res)
{
	switch (res.Type.Fmt)
	{
	case VariableFormat::Float:
		res.Value.Float4Val = arg1.Value.Float4Val * arg2.Value.Float4Val;
		break;
	case VariableFormat::Int:
		res.Value.Int4Val = arg1.Value.Int4Val * arg2.Value.Int4Val;
		break;
	case VariableFormat::Uint:
		res.Value.Uint4Val = arg1.Value.Uint4Val * arg2.Value.Uint4Val;
		break;
	case VariableFormat::Bool:
		AstError(n, "Bool multiply has not been defined");
		break;
	default:
		Unimplemented();
	}
}

void OperatorDivide(const Node* n, const Result& arg1, const Result& arg2, Result& res)
{
	for (u32 i = 0 ; i < res.Type.Dim ; ++i)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Int:
			AstAssert(n, arg2.Value.Int4Val.m[i] != 0, "Divide by zero");
			res.Value.Int4Val.m[i] = arg1.Value.Int4Val.m[i] / 
				arg2.Value.Int4Val.m[i];
			break;
		case VariableFormat::Uint:
			AstAssert(n, arg2.Value.Uint4Val.m[i] != 0, "Divide by zero");
			res.Value.Uint4Val.m[i] = arg1.Value.Uint4Val.m[i] / 
				arg2.Value.Uint4Val.m[i];
			break;
		case VariableFormat::Float:
			AstAssert(n, arg2.Value.Float4Val.m[i] != 0.f, "Divide by zero");
			res.Value.Float4Val.m[i] = arg1.Value.Float4Val.m[i] / 
				arg2.Value.Float4Val.m[i];
			break;
		case VariableFormat::Bool:
		default:
			Unimplemented();
		}
	}
}

// -----------------------------------------------------------------------------
// ------------------------------ NODE EVALS -----------------------------------
// -----------------------------------------------------------------------------
void FloatLiteral::Evaluate(const EvaluationContext&, Result& res) const
{
	res.Type = FloatType;
	res.Value.FloatVal = Val;
}

void Subscript::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result subjectRes;
	Subject->Evaluate(ec, subjectRes);
	AstAssert(this, subjectRes.Type.Fmt != VariableFormat::Bool,
		"Subscripts aren't usable on bool types");
	AstAssert(this, Index <= subjectRes.Type.Dim, "Invalid subscript for this type");
	AstAssert(this, subjectRes.Type.Fmt != VariableFormat::Float4x4,
		"Subscripts aren't usable on Matrix types");
	res.Type = FloatType;
	if (Index == 0)
		res.Value.FloatVal = subjectRes.Value.Float4Val.x;
	else if (Index == 1)
		res.Value.FloatVal = subjectRes.Value.Float4Val.y;
	else if (Index == 2)
		res.Value.FloatVal = subjectRes.Value.Float4Val.z;
	else if (Index == 3)
		res.Value.FloatVal = subjectRes.Value.Float4Val.w;
	else
		Unimplemented();
}

void BinaryOp::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	if (arg1Res.Type.Fmt == VariableFormat::Float4x4 ||
		arg2Res.Type.Fmt == VariableFormat::Float4x4)
	{
		AstAssert(this, OpType == Type::Multiply, "Matrices can only be multiplied");
		AstAssert(this, arg1Res.Type.Fmt == VariableFormat::Float4x4 &&
			arg2Res.Type.Fmt == VariableFormat::Float4x4, 
			"Matrix types can only be multiplied with other Matrix types");
		res.Type = Float4x4Type;
		res.Value.Float4x4Val = arg1Res.Value.Float4x4Val * arg2Res.Value.Float4x4Val;
		return;
	}
	if (OpType == Type::Divide)
	{
		AstAssert(this, arg1Res.Type.Fmt != VariableFormat::Bool && 
			arg2Res.Type.Fmt != VariableFormat::Bool,
			"Bool types not supported in divides.");
	}
	VariableType outType = DetermineResultType(this, arg1Res.Type, arg2Res.Type);
	Expand(arg1Res);
	Expand(arg2Res);
	Convert(arg1Res, outType.Fmt);
	Convert(arg2Res, outType.Fmt);
	res.Type = outType;
	if (OpType == Type::Add)
		OperatorAdd(this, arg1Res, arg2Res, res);
	else if (OpType == Type::Subtract)
		OperatorSubtract(this, arg1Res, arg2Res, res);
	else if (OpType == Type::Multiply)
		OperatorMultiply(this, arg1Res, arg2Res, res);
	else if (OpType == Type::Divide)
		OperatorDivide(this, arg1Res, arg2Res, res);
	else
		Unimplemented();

}

void TuneableRef::Evaluate(const EvaluationContext&, Result& res) const
{
	res.Type = Tune->Type;
	res.Value = Tune->Value;
}


// -----------------------------------------------------------------------------
// ------------------------------ FUNCTION EVALS -------------------------------
// -----------------------------------------------------------------------------
void EvaluateFloat3(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 3, "Float3 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
	Expect(n, FloatType, resX, "arg1");
	Expect(n, FloatType, resY, "arg2");
	Expect(n, FloatType, resZ, "arg3");
	res.Type = Float3Type;
	res.Value.Float3Val.x = resX.Value.FloatVal;
	res.Value.Float3Val.y = resY.Value.FloatVal;
	res.Value.Float3Val.z = resZ.Value.FloatVal;
}

void EvaluateFloat2(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 2, "Float2 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	Expect(n, FloatType, resX, "arg1");
	Expect(n, FloatType, resY, "arg2");
	res.Type = Float2Type;
	res.Value.Float3Val.x = resX.Value.FloatVal;
	res.Value.Float3Val.y = resY.Value.FloatVal;
}

void EvaluateTime(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 0, "Time does not take a param");
	res.Type = FloatType;
	res.Value.FloatVal = ec.Time;
}

void EvaluateDisplaySize(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 0, "DisplaySize does not take a param");
	res.Type = Uint2Type;
	res.Value.Uint4Val.x = ec.DisplaySize.x;
	res.Value.Uint4Val.y = ec.DisplaySize.y;
}

void EvaluateLookAt(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 2, "LookAt takes 2 params");
	Result fromRes, toRes;
	args[0]->Evaluate(ec, fromRes);
	args[1]->Evaluate(ec, toRes);
	Expect(n, Float3Type, fromRes, "arg1 (from)");
	Expect(n, Float3Type, toRes, "arg2 (to)");
	res.Type = Float4x4Type;
	res.Value.Float4x4Val = lookAt(fromRes.Value.Float3Val, toRes.Value.Float3Val);
}

void EvaluateProjection(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 4, "Projection takes 4 params");
	Result fovRes, aspectRes, nearRes, farRes;
	args[0]->Evaluate(ec, fovRes);
	args[1]->Evaluate(ec, aspectRes);
	args[2]->Evaluate(ec, nearRes);
	args[3]->Evaluate(ec, farRes);
	Expect(n, FloatType, fovRes, "arg1 (fov)");
	Expect(n, FloatType, aspectRes, "arg2 (aspect)");
	Expect(n, FloatType, nearRes, "arg3 (znear)");
	Expect(n, FloatType, farRes, "arg4 (zfar)");
	res.Type = Float4x4Type;
	res.Value.Float4x4Val = projection(fovRes.Value.FloatVal, aspectRes.Value.FloatVal, 
		nearRes.Value.FloatVal, farRes.Value.FloatVal);
}


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

typedef void (*FunctionEvaluate)(const Node*, const EvaluationContext&, std::vector<Node*>,
	Result&);
std::unordered_map<u32, FunctionEvaluate> FuncMap = {
	{ LowerHash("Float2"), EvaluateFloat2 },
	{ LowerHash("Float3"), EvaluateFloat3 },
	{ LowerHash("Time"), EvaluateTime },
	{ LowerHash("DisplaySize"), EvaluateDisplaySize },
	{ LowerHash("LookAt"), EvaluateLookAt },
	{ LowerHash("Projection"), EvaluateProjection },
};

void Function::Evaluate(const EvaluationContext& ec, Result& res) const
{
	u32 funcHash = LowerHash(Name.c_str());
	AstAssert(this, FuncMap.count(funcHash) == 1, "No function named %s exists.",
		Name.c_str());
	FunctionEvaluate fi = FuncMap[funcHash];
	fi(this, ec, Args, res);
}


#undef AstAssert


} // namespace ast
} // namespace rlf
