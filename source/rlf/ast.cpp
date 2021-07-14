
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



void Expect(VariableType type, Result& res, const char* name)
{
	AstAssert(res.Type == type, "Expected %s to be type %s but got %s.",
		name, TypeToString(type), TypeToString(res.Type));
}

VariableType DetermineResultType(VariableType t1, VariableType t2)
{
	AstAssert(t1.Dim == t2.Dim || t1.Dim == 1 || t2.Dim == 1, 
		"Vector size mismatch in multiply, %u and %u", t1.Dim, t2.Dim);
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
		case VariableFormat::Float:
			res.Value.Float4Val.w = res.Value.Float4Val.z = res.Value.Float4Val.y = 
				res.Value.Float4Val.x;
			break;
		case VariableFormat::Int:
			res.Value.Int4Val.w = res.Value.Int4Val.z = res.Value.Int4Val.y = 
				res.Value.Int4Val.x;
			break;
		case VariableFormat::Bool:
			res.Value.Uint4Val.w = res.Value.Uint4Val.z = res.Value.Uint4Val.y = 
				res.Value.Uint4Val.x = res.Value.BoolVal ? 1 : 0;
			break;
		case VariableFormat::Uint:
			res.Value.Uint4Val.w = res.Value.Uint4Val.z = res.Value.Uint4Val.y = 
				res.Value.Uint4Val.x;
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
	if (fmt == VariableFormat::Float)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Uint:
		case VariableFormat::Bool:
			res.Value.Float4Val.x = (float)res.Value.Uint4Val.x;
			res.Value.Float4Val.y = (float)res.Value.Uint4Val.y;
			res.Value.Float4Val.z = (float)res.Value.Uint4Val.z;
			res.Value.Float4Val.w = (float)res.Value.Uint4Val.w;
			break;
		case VariableFormat::Int:
			res.Value.Float4Val.x = (float)res.Value.Int4Val.x;
			res.Value.Float4Val.y = (float)res.Value.Int4Val.y;
			res.Value.Float4Val.z = (float)res.Value.Int4Val.z;
			res.Value.Float4Val.w = (float)res.Value.Int4Val.w;
			break;
		default:
			Unimplemented();
		}
	}
	else if (fmt == VariableFormat::Int)
	{
		switch (res.Type.Fmt)
		{
		case VariableFormat::Uint:
		case VariableFormat::Bool:
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
	AstAssert(subjectRes.Type.Fmt != VariableFormat::Bool, "Subscripts aren't usable on bool types");
	AstAssert(Index <= subjectRes.Type.Dim, "Invalid subscript for this type");
	AstAssert(subjectRes.Type.Fmt != VariableFormat::Float4x4,
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

void Multiply::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	if (arg1Res.Type.Fmt == VariableFormat::Float4x4 ||
		arg2Res.Type.Fmt == VariableFormat::Float4x4)
	{
		AstAssert(arg1Res.Type.Fmt == VariableFormat::Float4x4 &&
			arg2Res.Type.Fmt == VariableFormat::Float4x4, 
			"Matrix types can only be multiplied with other Matrix types");
		res.Type = Float4x4Type;
		res.Value.Float4x4Val = arg1Res.Value.Float4x4Val * arg2Res.Value.Float4x4Val;
		return;
	}
	VariableType outType = DetermineResultType(arg1Res.Type, arg2Res.Type);
	Expand(arg1Res);
	Expand(arg2Res);
	Convert(arg1Res, outType.Fmt);
	Convert(arg2Res, outType.Fmt);
	res.Type = outType;
	switch (outType.Fmt)
	{
	case VariableFormat::Float:
		res.Value.Float4Val = arg1Res.Value.Float4Val * arg2Res.Value.Float4Val;
		break;
	case VariableFormat::Int:
		res.Value.Int4Val = arg1Res.Value.Int4Val * arg2Res.Value.Int4Val;
		break;
	case VariableFormat::Uint:
	case VariableFormat::Bool:
		res.Value.Uint4Val = arg1Res.Value.Uint4Val * arg2Res.Value.Uint4Val;
		break;
	default:
		Unimplemented();
	}
}

void Divide::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, arg1Res);
	Arg2->Evaluate(ec, arg2Res);
	AstAssert(arg1Res.Type.Fmt != VariableFormat::Float4x4 && 
		arg2Res.Type.Fmt != VariableFormat::Float4x4,
		"Matrix types not supported in divides.");
	VariableType outType = DetermineResultType(arg1Res.Type, arg2Res.Type);
	Expand(arg1Res);
	Expand(arg2Res);
	Convert(arg1Res, outType.Fmt);
	Convert(arg2Res, outType.Fmt);
	res.Type = outType;
	switch (outType.Fmt)
	{
	case VariableFormat::Float:
		res.Value.Float4Val = arg1Res.Value.Float4Val / arg2Res.Value.Float4Val;
		break;
	case VariableFormat::Int:
		res.Value.Int4Val = arg1Res.Value.Int4Val / arg2Res.Value.Int4Val;
		break;
	case VariableFormat::Uint:
	case VariableFormat::Bool:
		res.Value.Uint4Val = arg1Res.Value.Uint4Val / arg2Res.Value.Uint4Val;
		break;
	default:
		Unimplemented();
	}
}

void TuneableRef::Evaluate(const EvaluationContext&, Result& res) const
{
	res.Type = Tune->Type;
	res.Value = Tune->Value;
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
	Expect(FloatType, resX, "arg1");
	Expect(FloatType, resY, "arg2");
	Expect(FloatType, resZ, "arg3");
	res.Type = Float3Type;
	res.Value.Float3Val.x = resX.Value.FloatVal;
	res.Value.Float3Val.y = resY.Value.FloatVal;
	res.Value.Float3Val.z = resZ.Value.FloatVal;
}

void EvaluateFloat2(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 2, "Float2 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	Expect(FloatType, resX, "arg1");
	Expect(FloatType, resY, "arg2");
	res.Type = Float2Type;
	res.Value.Float3Val.x = resX.Value.FloatVal;
	res.Value.Float3Val.y = resY.Value.FloatVal;
}

void EvaluateTime(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 0, "Time does not take a param");
	res.Type = FloatType;
	res.Value.FloatVal = ec.Time;
}

void EvaluateDisplaySize(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 0, "DisplaySize does not take a param");
	res.Type = Uint2Type;
	res.Value.Uint4Val.x = ec.DisplaySize.x;
	res.Value.Uint4Val.y = ec.DisplaySize.y;
}

void EvaluateLookAt(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 2, "LookAt takes 2 params");
	Result fromRes, toRes;
	args[0]->Evaluate(ec, fromRes);
	args[1]->Evaluate(ec, toRes);
	Expect(Float3Type, fromRes, "arg1 (from)");
	Expect(Float3Type, toRes, "arg2 (to)");
	res.Type = Float4x4Type;
	res.Value.Float4x4Val = lookAt(fromRes.Value.Float3Val, toRes.Value.Float3Val);
}

void EvaluateProjection(const EvaluationContext& ec, std::vector<Node*> args, Result& res)
{
	AstAssert(args.size() == 4, "Projection takes 4 params");
	Result fovRes, aspectRes, nearRes, farRes;
	args[0]->Evaluate(ec, fovRes);
	args[1]->Evaluate(ec, aspectRes);
	args[2]->Evaluate(ec, nearRes);
	args[3]->Evaluate(ec, farRes);
	Expect(FloatType, fovRes, "arg1 (fov)");
	Expect(FloatType, aspectRes, "arg2 (aspect)");
	Expect(FloatType, nearRes, "arg3 (znear)");
	Expect(FloatType, farRes, "arg4 (zfar)");
	res.Type = Float4x4Type;
	res.Value.Float4x4Val = projection(fovRes.Value.FloatVal, aspectRes.Value.FloatVal, 
		nearRes.Value.FloatVal, farRes.Value.FloatVal);
}

#undef AstAssert


} // namespace ast
} // namespace rlf
