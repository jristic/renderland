
namespace rlf {
namespace ast {


#define NODE_TYPE_ENTRY(type, eval_func, dep_func) \
	static_assert(offsetof(type, Common) == 0, \
		"Common must be first member of AST nodes for pointer casting to work.");
NODE_TYPE_TUPLE
#undef NODE_TYPE_ENTRY


void Evaluate(const Node* node, const EvaluationContext& ec, Result& res);

void Evaluate(const EvaluationContext& ec, Expression& expr, Result& res, 
	ErrorState& es)
{
	es.Success = true;

	if ((expr.Dep.VariesByFlags & ec.ChangedThisFrameFlags) == 0 && expr.CacheValid)
	{
		res = expr.CachedResult;
		return;
	}

	try {
		Evaluate(expr.TopNode, ec, res);
	}
	catch (ErrorInfo ae)
	{
		es.Success = false;
		es.Info = ae;
	}

	expr.CachedResult = res;
	expr.CacheValid = true;
}

void AstError(const Node* n, const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	ErrorInfo ae;
	ae.Location = n->Location;
	ae.Message = buf;
	throw ae;
}

#define AstAssert(node, expression, message, ...) 	\
do {												\
	if (!(expression)) {							\
		AstError(node, message, ##__VA_ARGS__);		\
	}												\
} while (0);										\



void ExpectFmt(const Node* n, VariableFormat fmt, Result& res, const char* name)
{
	AstAssert(n, res.Type.Fmt == fmt, "Expected %s to be type %s but got %s.",
		name, TypeFmtToString(fmt), TypeFmtToString(res.Type.Fmt));
}

void ExpectDim(const Node* n, u32 dim, Result& res, const char* name)
{
	AstAssert(n, res.Type.Dim == dim, "Expected %s to be size %u but got %u.",
		name, dim, res.Type.Dim);
}

VariableFormat DetermineResultFormat(VariableFormat f1, VariableFormat f2)
{
	if (f1 == VariableFormat::Float || f2 == VariableFormat::Float)
		return VariableFormat::Float;
	else if (f1 == VariableFormat::Int || f2 == VariableFormat::Int)
		return VariableFormat::Int;
	else if (f1 == VariableFormat::Uint || f2 == VariableFormat::Uint)
		return VariableFormat::Uint;
	else
		return VariableFormat::Bool;
}

VariableType DetermineResultType(const Node* n, VariableType t1, VariableType t2)
{
	AstAssert(n, t1.Dim == t2.Dim || t1.Dim == 1 || t2.Dim == 1, 
		"Vector size mismatch in operation, %u vs. %u", t1.Dim, t2.Dim);
	VariableType t;
	t.Dim = t1.Dim == 1 ? t2.Dim : t1.Dim;
	t.Fmt = DetermineResultFormat(t1.Fmt, t2.Fmt);
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
void UintLiteral_Evaluate(const Node* n, const EvaluationContext&, Result& res)
{
	UintLiteral* node = (UintLiteral*)n;
	res.Type = UintType;
	res.Value.UintVal = node->Val;
}

void IntLiteral_Evaluate(const Node* n, const EvaluationContext&, Result& res)
{
	IntLiteral* node = (IntLiteral*)n;
	res.Type = IntType;
	res.Value.IntVal = node->Val;
}

void FloatLiteral_Evaluate(const Node* n, const EvaluationContext&, Result& res)
{
	FloatLiteral* node = (FloatLiteral*)n;
	res.Type = FloatType;
	res.Value.FloatVal = node->Val;
}

void Subscript_Evaluate(const Node* n, const EvaluationContext& ec, Result& res)
{
	Subscript* node = (Subscript*)n;
	Result subjectRes;
	Evaluate(node->Subject, ec, subjectRes);
	AstAssert(n, subjectRes.Type.Fmt != VariableFormat::Float4x4,
		"Subscripts aren't usable on Matrix types");
	res.Type.Fmt = subjectRes.Type.Fmt;
	u32 dim = 0;
	for (u32 i = 0 ; i < 4 ; ++i)
	{
		if (node->Index[i] == 0)
			break;
		AstAssert(n, node->Index[i] <= subjectRes.Type.Dim, 
			"Invalid subscript for this type");
		Assert(node->Index[i] <= 4, "Invalid subscript");
		u8* src = ((u8*)&subjectRes.Value) + 4*(node->Index[i]-1);
		u8* dest = ((u8*)&res.Value) + 4*i;
		memcpy(dest, src, 4);
		++dim;
	}
	Assert(dim > 0, "There must've been at least one subscript element.");
	res.Type.Dim = dim;
}
void Subscript_GetDependency(const Node* n, DependencyInfo& dep)
{
	Subscript* node = (Subscript*)n;	
	GetDependency(node->Subject, dep);
}

void Group_Evaluate(const Node* n, const EvaluationContext& ec, Result& res)
{
	Group* node = (Group*)n;
	Evaluate(node->Sub, ec, res);
}
void Group_GetDependency(const Node* n, DependencyInfo& dep)
{
	Group* node = (Group*)n;
	GetDependency(node->Sub, dep);
}

void BinaryOp_Evaluate(const Node* n, const EvaluationContext& ec, Result& outRes)
{
	BinaryOp* node = (BinaryOp*)n;
	Result res, arg1Res, arg2Res;
	Evaluate(node->LArg, ec, arg1Res);
	Evaluate(node->RArg, ec, arg2Res);
	if (arg1Res.Type.Fmt == VariableFormat::Float4x4 ||
		arg2Res.Type.Fmt == VariableFormat::Float4x4)
	{
		AstAssert(n, node->Op == BinaryOp::Type::Multiply, "Matrices can only be multiplied");
		AstAssert(n, arg1Res.Type.Fmt == VariableFormat::Float4x4 &&
			arg2Res.Type.Fmt == VariableFormat::Float4x4, 
			"Matrix types can only be multiplied with other Matrix types");
		res.Type = Float4x4Type;
		res.Value.Float4x4Val = arg1Res.Value.Float4x4Val * arg2Res.Value.Float4x4Val;
	}
	else
	{
		if (node->Op == BinaryOp::Type::Divide)
		{
			AstAssert(n, arg1Res.Type.Fmt != VariableFormat::Bool && 
				arg2Res.Type.Fmt != VariableFormat::Bool,
				"Bool types not supported in divides.");
		}
		VariableType outType = DetermineResultType(n, arg1Res.Type, arg2Res.Type);
		Expand(arg1Res);
		Expand(arg2Res);
		Convert(arg1Res, outType.Fmt);
		Convert(arg2Res, outType.Fmt);
		res.Type = outType;
		if (node->Op == BinaryOp::Type::Add)
			OperatorAdd(n, arg1Res, arg2Res, res);
		else if (node->Op == BinaryOp::Type::Subtract)
			OperatorSubtract(n, arg1Res, arg2Res, res);
		else if (node->Op == BinaryOp::Type::Multiply)
			OperatorMultiply(n, arg1Res, arg2Res, res);
		else if (node->Op == BinaryOp::Type::Divide)
			OperatorDivide(n, arg1Res, arg2Res, res);
		else
			Unimplemented();
	}
	outRes = res;
}
void BinaryOp_GetDependency(const Node* n, DependencyInfo& dep)
{
	BinaryOp* node = (BinaryOp*)n;
	GetDependency(node->LArg, dep);
	GetDependency(node->RArg, dep);
}

void Join_Evaluate(const Node* n, const EvaluationContext& ec, Result& outRes)
{
	Join* j = (Join*)n;
	Assert(j->Comps.Count > 0, "Invalid join");
	Result res = {};
	Evaluate(j->Comps[0], ec, res);
	for (size_t i = 1 ; i < j->Comps.Count ; ++i)
	{
		Result jr;
		Evaluate(j->Comps[i], ec, jr);
		VariableFormat jf = DetermineResultFormat(res.Type.Fmt, jr.Type.Fmt);
		u32 jd = res.Type.Dim + jr.Type.Dim;
		AstAssert(n, jd <= 4, "Resulting vector of size %d is unsupported", jd);
		Convert(res, jf);
		Convert(jr, jf);
		for (u32 k = res.Type.Dim, l = 0 ; k < jd ; ++k, ++l)
		{
			res.Value.Float4Val.m[k] = jr.Value.Float4Val.m[l];
		}
		res.Type.Fmt = jf;
		res.Type.Dim = jd;
	}
	outRes = res;
}
void Join_GetDependency(const Node* n, DependencyInfo& dep)
{
	Join* j = (Join*)n;
	for (Node* comp : j->Comps)
		GetDependency(comp, dep);
}

void VariableRef_Evaluate(const Node* n, const EvaluationContext&, Result& res)
{
	VariableRef* vr = (VariableRef*)n;
	if (vr->IsTuneable)
	{
		Tuneable* tune = (Tuneable*)vr->M;
		res.Type = tune->Type;
		res.Value = tune->Value;
	}
	else
	{
		rlf::Constant* cnst = (rlf::Constant*)vr->M;
		res.Type = cnst->Type;
		res.Value = cnst->Value;
	}
}
void VariableRef_GetDependency(const Node* n, DependencyInfo& dep)
{
	VariableRef* vr = (VariableRef*)n;
	if (vr->IsTuneable)
	{
		dep.VariesByFlags |= VariesBy_Tuneable;
	}
	else
	{
		rlf::Constant* cnst = (rlf::Constant*)vr->M;
		dep.VariesByFlags |= cnst->Expr.Dep.VariesByFlags;
	}
}


// -----------------------------------------------------------------------------
// ------------------------------ FUNCTION EVALS -------------------------------
// -----------------------------------------------------------------------------
void EvaluateInt(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 1, "Int takes 1 param.");
	Result resX;
	Evaluate(args[0], ec, resX);
	ExpectDim(n, 1, resX, "arg1");
	Convert(resX, VariableFormat::Int);
	res.Type = IntType;
	res.Value.IntVal = resX.Value.IntVal;
}

void EvaluateInt2(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 2, "Int2 takes 2 params.");
	Result resX, resY;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	Convert(resX, VariableFormat::Int);
	Convert(resY, VariableFormat::Int);
	res.Type = Int2Type;
	res.Value.Int4Val.x = resX.Value.IntVal;
	res.Value.Int4Val.y = resY.Value.IntVal;
}

void EvaluateInt3(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 3, "Int3 takes 3 params.");
	Result resX, resY, resZ;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	Evaluate(args[2], ec, resZ);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	ExpectDim(n, 1, resZ, "arg3");
	Convert(resX, VariableFormat::Int);
	Convert(resY, VariableFormat::Int);
	Convert(resZ, VariableFormat::Int);
	res.Type = Int3Type;
	res.Value.Int4Val.x = resX.Value.IntVal;
	res.Value.Int4Val.y = resY.Value.IntVal;
	res.Value.Int4Val.z = resZ.Value.IntVal;
}

void EvaluateInt4(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 4, "Int4 takes 4 params.");
	Result resX, resY, resZ, resW;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	Evaluate(args[2], ec, resZ);
	Evaluate(args[3], ec, resW);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	ExpectDim(n, 1, resZ, "arg3");
	ExpectDim(n, 1, resW, "arg4");
	Convert(resX, VariableFormat::Int);
	Convert(resY, VariableFormat::Int);
	Convert(resZ, VariableFormat::Int);
	Convert(resW, VariableFormat::Int);
	res.Type = Int3Type;
	res.Value.Int4Val.x = resX.Value.IntVal;
	res.Value.Int4Val.y = resY.Value.IntVal;
	res.Value.Int4Val.z = resZ.Value.IntVal;
	res.Value.Int4Val.w = resW.Value.IntVal;
}

void EvaluateUint(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 1, "Uint takes 1 param.");
	Result resX;
	Evaluate(args[0], ec, resX);
	ExpectDim(n, 1, resX, "arg1");
	Convert(resX, VariableFormat::Uint);
	res.Type = UintType;
	res.Value.UintVal = resX.Value.UintVal;
}

void EvaluateUint2(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 2, "Uint2 takes 2 params.");
	Result resX, resY;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	Convert(resX, VariableFormat::Uint);
	Convert(resY, VariableFormat::Uint);
	res.Type = Uint2Type;
	res.Value.Uint4Val.x = resX.Value.UintVal;
	res.Value.Uint4Val.y = resY.Value.UintVal;
}

void EvaluateUint3(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 3, "Uint3 takes 3 params.");
	Result resX, resY, resZ;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	Evaluate(args[2], ec, resZ);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	ExpectDim(n, 1, resZ, "arg3");
	Convert(resX, VariableFormat::Uint);
	Convert(resY, VariableFormat::Uint);
	Convert(resZ, VariableFormat::Uint);
	res.Type = Uint3Type;
	res.Value.Uint4Val.x = resX.Value.UintVal;
	res.Value.Uint4Val.y = resY.Value.UintVal;
	res.Value.Uint4Val.z = resZ.Value.UintVal;
}

void EvaluateUint4(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 4, "Uint4 takes 4 params.");
	Result resX, resY, resZ, resW;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	Evaluate(args[2], ec, resZ);
	Evaluate(args[3], ec, resW);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	ExpectDim(n, 1, resZ, "arg3");
	ExpectDim(n, 1, resW, "arg4");
	Convert(resX, VariableFormat::Uint);
	Convert(resY, VariableFormat::Uint);
	Convert(resZ, VariableFormat::Uint);
	Convert(resW, VariableFormat::Uint);
	res.Type = Uint3Type;
	res.Value.Uint4Val.x = resX.Value.UintVal;
	res.Value.Uint4Val.y = resY.Value.UintVal;
	res.Value.Uint4Val.z = resZ.Value.UintVal;
	res.Value.Uint4Val.w = resW.Value.UintVal;
}

void EvaluateFloat(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 1, "Float takes 1 param.");
	Result resX;
	Evaluate(args[0], ec, resX);
	ExpectDim(n, 1, resX, "arg1");
	Convert(resX, VariableFormat::Float);
	res.Type = FloatType;
	res.Value.FloatVal = resX.Value.FloatVal;
}

void EvaluateFloat2(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 2, "Float2 takes 2 params.");
	Result resX, resY;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	Convert(resX, VariableFormat::Float);
	Convert(resY, VariableFormat::Float);
	res.Type = Float2Type;
	res.Value.Float2Val.x = resX.Value.FloatVal;
	res.Value.Float2Val.y = resY.Value.FloatVal;
}

void EvaluateFloat3(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 3, "Float3 takes 3 params.");
	Result resX, resY, resZ;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	Evaluate(args[2], ec, resZ);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	ExpectDim(n, 1, resZ, "arg3");
	Convert(resX, VariableFormat::Float);
	Convert(resY, VariableFormat::Float);
	Convert(resZ, VariableFormat::Float);
	res.Type = Float3Type;
	res.Value.Float3Val.x = resX.Value.FloatVal;
	res.Value.Float3Val.y = resY.Value.FloatVal;
	res.Value.Float3Val.z = resZ.Value.FloatVal;
}

void EvaluateFloat4(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 4, "Float4 takes 4 params.");
	Result resX, resY, resZ, resW;
	Evaluate(args[0], ec, resX);
	Evaluate(args[1], ec, resY);
	Evaluate(args[2], ec, resZ);
	Evaluate(args[3], ec, resW);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	ExpectDim(n, 1, resZ, "arg3");
	ExpectDim(n, 1, resW, "arg4");
	Convert(resX, VariableFormat::Float);
	Convert(resY, VariableFormat::Float);
	Convert(resZ, VariableFormat::Float);
	Convert(resW, VariableFormat::Float);
	res.Type = Float3Type;
	res.Value.Float4Val.x = resX.Value.FloatVal;
	res.Value.Float4Val.y = resY.Value.FloatVal;
	res.Value.Float4Val.z = resZ.Value.FloatVal;
	res.Value.Float4Val.w = resW.Value.FloatVal;
}

void EvaluateTime(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 0, "Time does not take a param");
	res.Type = FloatType;
	res.Value.FloatVal = ec.Time;
}

void EvaluateDisplaySize(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 0, "DisplaySize does not take a param");
	res.Type = Uint2Type;
	res.Value.Uint4Val.x = ec.DisplaySize.x;
	res.Value.Uint4Val.y = ec.DisplaySize.y;
}

void EvaluateSin(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 1, "Sin takes 1 param");
	Result argRes;
	Evaluate(args[0], ec, argRes);
	Convert(argRes, VariableFormat::Float);
	res.Type = argRes.Type;
	for (u32 i = 0 ; i < argRes.Type.Dim ; ++i)
	{
		res.Value.Float4Val.m[i] = sin(argRes.Value.Float4Val.m[i]);
	}
}

void EvaluateCos(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 1, "Cos takes 1 param");
	Result argRes;
	Evaluate(args[0], ec, argRes);
	Convert(argRes, VariableFormat::Float);
	res.Type = argRes.Type;
	for (u32 i = 0 ; i < argRes.Type.Dim ; ++i)
	{
		res.Value.Float4Val.m[i] = cos(argRes.Value.Float4Val.m[i]);
	}
}

void EvaluateMin(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 2, "Min takes 2 params");
	Result arg1Res;
	Evaluate(args[0], ec, arg1Res);
	AstAssert(n, arg1Res.Type != Float4x4Type, "Min does not accept matrices (arg1)");
	Result arg2Res;
	Evaluate(args[1], ec, arg2Res);
	AstAssert(n, arg1Res.Type != Float4x4Type, "Min does not accept matrices (arg2)");

	VariableType outType = DetermineResultType(n, arg1Res.Type, arg2Res.Type);
	Expand(arg1Res);
	Expand(arg2Res);
	Convert(arg1Res, outType.Fmt);
	Convert(arg2Res, outType.Fmt);
	res.Type = outType;

	if (outType.Fmt == VariableFormat::Bool) {
		res.Value.Bool4Val.x = arg1Res.Value.Bool4Val.x && arg2Res.Value.Bool4Val.x;
		res.Value.Bool4Val.y = arg1Res.Value.Bool4Val.y && arg2Res.Value.Bool4Val.y;
		res.Value.Bool4Val.z = arg1Res.Value.Bool4Val.z && arg2Res.Value.Bool4Val.z;
		res.Value.Bool4Val.w = arg1Res.Value.Bool4Val.w && arg2Res.Value.Bool4Val.w;
	}
	else  if (outType.Fmt == VariableFormat::Int) {
		res.Value.Int4Val.x = min(arg1Res.Value.Int4Val.x, arg2Res.Value.Int4Val.x);
		res.Value.Int4Val.y = min(arg1Res.Value.Int4Val.y, arg2Res.Value.Int4Val.y);
		res.Value.Int4Val.z = min(arg1Res.Value.Int4Val.z, arg2Res.Value.Int4Val.z);
		res.Value.Int4Val.w = min(arg1Res.Value.Int4Val.w, arg2Res.Value.Int4Val.w);
	}
	else  if (outType.Fmt == VariableFormat::Uint) {
		res.Value.Uint4Val.x = min(arg1Res.Value.Uint4Val.x, arg2Res.Value.Uint4Val.x);
		res.Value.Uint4Val.y = min(arg1Res.Value.Uint4Val.y, arg2Res.Value.Uint4Val.y);
		res.Value.Uint4Val.z = min(arg1Res.Value.Uint4Val.z, arg2Res.Value.Uint4Val.z);
		res.Value.Uint4Val.w = min(arg1Res.Value.Uint4Val.w, arg2Res.Value.Uint4Val.w);
	}
	else  if (outType.Fmt == VariableFormat::Float) {
		res.Value.Float4Val.x = min(arg1Res.Value.Float4Val.x, arg2Res.Value.Float4Val.x);
		res.Value.Float4Val.y = min(arg1Res.Value.Float4Val.y, arg2Res.Value.Float4Val.y);
		res.Value.Float4Val.z = min(arg1Res.Value.Float4Val.z, arg2Res.Value.Float4Val.z);
		res.Value.Float4Val.w = min(arg1Res.Value.Float4Val.w, arg2Res.Value.Float4Val.w);
	}
	else
		Unimplemented();
}

void EvaluateMax(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 2, "Max takes 2 params");
	Result arg1Res;
	Evaluate(args[0], ec, arg1Res);
	AstAssert(n, arg1Res.Type != Float4x4Type, "Max does not accept matrices (arg1)");
	Result arg2Res;
	Evaluate(args[1], ec, arg2Res);
	AstAssert(n, arg2Res.Type != Float4x4Type, "Max does not accept matrices (arg2)");

	VariableType outType = DetermineResultType(n, arg1Res.Type, arg2Res.Type);
	Expand(arg1Res);
	Expand(arg2Res);
	Convert(arg1Res, outType.Fmt);
	Convert(arg2Res, outType.Fmt);
	res.Type = outType;

	if (outType.Fmt == VariableFormat::Bool) {
		res.Value.Bool4Val.x = arg1Res.Value.Bool4Val.x || arg2Res.Value.Bool4Val.x;
		res.Value.Bool4Val.y = arg1Res.Value.Bool4Val.y || arg2Res.Value.Bool4Val.y;
		res.Value.Bool4Val.z = arg1Res.Value.Bool4Val.z || arg2Res.Value.Bool4Val.z;
		res.Value.Bool4Val.w = arg1Res.Value.Bool4Val.w || arg2Res.Value.Bool4Val.w;
	}
	else  if (outType.Fmt == VariableFormat::Int) {
		res.Value.Int4Val.x = max(arg1Res.Value.Int4Val.x, arg2Res.Value.Int4Val.x);
		res.Value.Int4Val.y = max(arg1Res.Value.Int4Val.y, arg2Res.Value.Int4Val.y);
		res.Value.Int4Val.z = max(arg1Res.Value.Int4Val.z, arg2Res.Value.Int4Val.z);
		res.Value.Int4Val.w = max(arg1Res.Value.Int4Val.w, arg2Res.Value.Int4Val.w);
	}
	else  if (outType.Fmt == VariableFormat::Uint) {
		res.Value.Uint4Val.x = max(arg1Res.Value.Uint4Val.x, arg2Res.Value.Uint4Val.x);
		res.Value.Uint4Val.y = max(arg1Res.Value.Uint4Val.y, arg2Res.Value.Uint4Val.y);
		res.Value.Uint4Val.z = max(arg1Res.Value.Uint4Val.z, arg2Res.Value.Uint4Val.z);
		res.Value.Uint4Val.w = max(arg1Res.Value.Uint4Val.w, arg2Res.Value.Uint4Val.w);
	}
	else  if (outType.Fmt == VariableFormat::Float) {
		res.Value.Float4Val.x = max(arg1Res.Value.Float4Val.x, arg2Res.Value.Float4Val.x);
		res.Value.Float4Val.y = max(arg1Res.Value.Float4Val.y, arg2Res.Value.Float4Val.y);
		res.Value.Float4Val.z = max(arg1Res.Value.Float4Val.z, arg2Res.Value.Float4Val.z);
		res.Value.Float4Val.w = max(arg1Res.Value.Float4Val.w, arg2Res.Value.Float4Val.w);
	}
	else
		Unimplemented();
}

void EvaluateInverse(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 1, "Inverse takes 1 param");
	Result argRes;
	Evaluate(args[0], ec, argRes);
	ExpectFmt(n, VariableFormat::Float4x4, argRes, "arg1");
	res.Type = Float4x4Type;
	inverse(argRes.Value.Float4x4Val, res.Value.Float4x4Val);
}

void EvaluateLookAt(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 2, "LookAt takes 2 params");
	Result fromRes, toRes;
	Evaluate(args[0], ec, fromRes);
	Evaluate(args[1], ec, toRes);
	ExpectDim(n, 3, fromRes, "arg1 (from)");
	ExpectDim(n, 3, toRes, "arg2 (to)");
	Convert(fromRes, VariableFormat::Float);
	Convert(toRes, VariableFormat::Float);
	res.Type = Float4x4Type;
	res.Value.Float4x4Val = lookAt(fromRes.Value.Float3Val, toRes.Value.Float3Val);
}

void EvaluateProjection(const Node* n, const EvaluationContext& ec, Array<Node*> args,
	Result& res)
{
	AstAssert(n, args.Count == 4, "Projection takes 4 params");
	Result fovRes, aspectRes, nearRes, farRes;
	Evaluate(args[0], ec, fovRes);
	Evaluate(args[1], ec, aspectRes);
	Evaluate(args[2], ec, nearRes);
	Evaluate(args[3], ec, farRes);
	ExpectDim(n, 1, fovRes, "arg1 (fov)");
	ExpectDim(n, 1, aspectRes, "arg2 (aspect)");
	ExpectDim(n, 1, nearRes, "arg3 (znear)");
	ExpectDim(n, 1, farRes, "arg4 (zfar)");
	Convert(fovRes, VariableFormat::Float);
	Convert(aspectRes, VariableFormat::Float);
	Convert(nearRes, VariableFormat::Float);
	Convert(farRes, VariableFormat::Float);
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

typedef void (*FunctionEvaluate)(const Node*, const EvaluationContext&, Array<Node*>,
	Result&);
struct FunctionInfo
{
	FunctionEvaluate fn;
	VariesBy vb;
};
std::unordered_map<u32, FunctionInfo> FuncMap = {
	{ LowerHash("Int"), { EvaluateInt, VariesBy_None} },
	{ LowerHash("Int2"), { EvaluateInt2, VariesBy_None} },
	{ LowerHash("Int3"), { EvaluateInt3, VariesBy_None} },
	{ LowerHash("Int4"), { EvaluateInt4, VariesBy_None} },
	{ LowerHash("Uint"), { EvaluateUint, VariesBy_None} },
	{ LowerHash("Uint2"), { EvaluateUint2, VariesBy_None} },
	{ LowerHash("Uint3"), { EvaluateUint3, VariesBy_None} },
	{ LowerHash("Uint4"), { EvaluateUint4, VariesBy_None} },
	{ LowerHash("Float"), { EvaluateFloat, VariesBy_None} },
	{ LowerHash("Float2"), { EvaluateFloat2, VariesBy_None} },
	{ LowerHash("Float3"), { EvaluateFloat3, VariesBy_None} },
	{ LowerHash("Float4"), { EvaluateFloat4, VariesBy_None} },
	{ LowerHash("Time"), { EvaluateTime, VariesBy_Time} },
	{ LowerHash("DisplaySize"), { EvaluateDisplaySize, VariesBy_DisplaySize} },
	{ LowerHash("Sin"), { EvaluateSin, VariesBy_None} },
	{ LowerHash("Cos"), { EvaluateCos, VariesBy_None} },
	{ LowerHash("Min"), { EvaluateMin, VariesBy_None} },
	{ LowerHash("Max"), { EvaluateMax, VariesBy_None} },
	{ LowerHash("Inverse"), { EvaluateInverse, VariesBy_None} },
	{ LowerHash("LookAt"), { EvaluateLookAt, VariesBy_None} },
	{ LowerHash("Projection"), { EvaluateProjection, VariesBy_None} },
};

void Function_Evaluate(const Node* n, const EvaluationContext& ec, Result& res)
{
	Function* f = (Function*)n;
	u32 funcHash = LowerHash(f->Name);
	AstAssert(n, FuncMap.count(funcHash) == 1, "No function named %s exists.",
		f->Name);
	FunctionEvaluate fi = FuncMap[funcHash].fn;
	fi(n, ec, f->Args, res);
}
void Function_GetDependency(const Node* n, DependencyInfo& dep)
{
	Function* f = (Function*)n;
	u32 funcHash = LowerHash(f->Name);
	AstAssert(n, FuncMap.count(funcHash) == 1, "No function named %s exists.",
		f->Name);

	VariesBy vb = FuncMap[funcHash].vb;
	dep.VariesByFlags |= vb;

	for (Node* arg : f->Args)
		GetDependency(arg, dep);
}


void SizeOf_Evaluate(const Node* n, const EvaluationContext&, Result& res)
{
	SizeOf* sz = (SizeOf*)n;
	res.Type = 	UintType; 
	res.Value.UintVal = sz->Size;
}

void None_GetDependency(const Node*, DependencyInfo&)
{
	// Intentionally add no dependency
}


void Evaluate(const Node* node, const EvaluationContext& ec, Result& res)
{
	typedef void (*EvalFunc)(const Node*, const EvaluationContext&, Result&);

#define NODE_TYPE_ENTRY(type, eval_func, dep_func) eval_func,
	static EvalFunc EvalFuncs[] = 
	{
		NODE_TYPE_TUPLE
	};
#undef NODE_TYPE_ENTRY
	EvalFunc ef = EvalFuncs[(u32)node->Type];
	ef(node, ec, res);
}

void GetDependency(const Node* node, DependencyInfo& dep)
{
	typedef void (*DepFunc)(const Node*, DependencyInfo&);

#define NODE_TYPE_ENTRY(type, eval_func, dep_func) dep_func,
	static DepFunc DepFuncs[] = 
	{
		NODE_TYPE_TUPLE
	};
#undef NODE_TYPE_ENTRY
	DepFunc df = DepFuncs[(u32)node->Type];
	df(node, dep);
}


#undef AstAssert


} // namespace ast
} // namespace rlf
