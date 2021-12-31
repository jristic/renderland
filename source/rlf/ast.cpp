
namespace rlf {
namespace ast {


struct AstException
{
	ErrorInfo Info;
};

void Evaluate(const EvaluationContext& ec, Node* ast, Result& res, 
	EvaluateErrorState& es)
{
	es.EvaluateSuccess = true;

	if (ast->Constant() && ast->CacheValid)
	{
		res = ast->CachedResult;
		return;
	}

	try {
		ast->Evaluate(ec, res);
	}
	catch (AstException ae)
	{
		es.EvaluateSuccess = false;
		es.Info = ae.Info;
	}

	ast->CachedResult = res;
	ast->CacheValid = true;
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
void FloatLiteral::Evaluate(const EvaluationContext&, Result& res) const
{
	res.Type = FloatType;
	res.Value.FloatVal = Val;
}
void FloatLiteral::GetDependency(DependencyInfo& dep) const
{
	// Add no dependecies
	(void)dep;
}

void Subscript::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Result subjectRes;
	Subject->Evaluate(ec, subjectRes);
	AstAssert(this, subjectRes.Type.Fmt != VariableFormat::Float4x4,
		"Subscripts aren't usable on Matrix types");
	res.Type.Fmt = subjectRes.Type.Fmt;
	u32 dim = 0;
	for (int i = 0 ; i < 4 ; ++i)
	{
		if (Index[i] < 0)
			break;
		AstAssert(this, (u8)Index[i] < subjectRes.Type.Dim, "Invalid subscript for this type");
		Assert(Index[i] < 4, "Invalid subscript");
		u8* src = ((u8*)&subjectRes.Value) + 4*Index[i];
		u8* dest = ((u8*)&res.Value) + 4*i;
		memcpy(dest, src, 4);
		++dim;
	}
	Assert(dim > 0, "There must've been at least one subscript element.");
	res.Type.Dim = dim;
}
void Subscript::GetDependency(DependencyInfo& dep) const
{
	Subject->GetDependency(dep);
}

void Group::Evaluate(const EvaluationContext& ec, Result& res) const
{
	Sub->Evaluate(ec, res);
}
void Group::GetDependency(DependencyInfo& dep) const
{
	Sub->GetDependency(dep);
}

void BinaryOp::Evaluate(const EvaluationContext& ec, Result& outRes) const
{
	Assert(Ops.size() == Args.size() - 1, "Mismatched params");
	std::vector<Result> results(Args.size());
	std::vector<Type> ops(Ops.rbegin(), Ops.rend());
	for (size_t i = 0 ; i < Args.size() ; ++i)
	{
		Node* arg = Args[Args.size() - 1 - i];
		arg->Evaluate(ec, results[i]);
	}

	std::pair<Type,Type> OpPhase[] = {
		{ Type::Multiply, Type::Divide },
		{ Type::Add, Type::Subtract },
	};
	
	for (u32 p = 0 ; p < 2 ; ++p)
	{
		auto phase = OpPhase[p];
		for (size_t i = 0 ; i < ops.size() ; )
		{
			Type OpType = ops[i];
			if (OpType != phase.first && OpType != phase.second)
			{
				++i;
				continue;
			}
			Result res, arg1Res, arg2Res;
			arg1Res = results[i];
			arg2Res = results[i+1];
			if (arg1Res.Type.Fmt == VariableFormat::Float4x4 ||
				arg2Res.Type.Fmt == VariableFormat::Float4x4)
			{
				AstAssert(this, OpType == Type::Multiply, "Matrices can only be multiplied");
				AstAssert(this, arg1Res.Type.Fmt == VariableFormat::Float4x4 &&
					arg2Res.Type.Fmt == VariableFormat::Float4x4, 
					"Matrix types can only be multiplied with other Matrix types");
				res.Type = Float4x4Type;
				res.Value.Float4x4Val = arg1Res.Value.Float4x4Val * arg2Res.Value.Float4x4Val;
			}
			else
			{
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
			results[i+1] = res;
			results.erase(results.begin()+i);
			ops.erase(ops.begin()+i);
		}
	}
	Assert(results.size() == 1, "Should have been reduced to just one result");
	outRes = results.front();
}
void BinaryOp::GetDependency(DependencyInfo& dep) const
{
	for (Node* arg : Args)
		arg->GetDependency(dep);
}

void Join::Evaluate(const EvaluationContext& ec, Result& outRes) const
{
	Assert(Comps.size() > 0, "Invalid join");
	Result res = {};
	Comps[0]->Evaluate(ec, res);
	for (size_t i = 1 ; i < Comps.size() ; ++i)
	{
		Result jr;
		Comps[i]->Evaluate(ec, jr);
		VariableFormat jf = DetermineResultFormat(res.Type.Fmt, jr.Type.Fmt);
		u32 jd = res.Type.Dim + jr.Type.Dim;
		AstAssert(this, jd <= 4, "Resulting vector of size %d is unsupported", jd);
		for (u32 j = res.Type.Dim, k = 0 ; j < jd ; ++j, ++k)
		{
			res.Value.Float4Val.m[j] = jr.Value.Float4Val.m[k];
		}
		res.Type.Fmt = jf;
		res.Type.Dim = jd;
	}
	outRes = res;
}
void Join::GetDependency(DependencyInfo& dep) const
{
	for (Node* comp : Comps)
		comp->GetDependency(dep);
}

void VariableRef::Evaluate(const EvaluationContext&, Result& res) const
{
	if (isTuneable)
	{
		Tuneable* tune = (Tuneable*)M;
		res.Type = tune->Type;
		res.Value = tune->Value;
	}
	else
	{
		rlf::Constant* cnst = (rlf::Constant*)M;
		res.Type = cnst->Type;
		res.Value = cnst->Value;
	}
}
void VariableRef::GetDependency(DependencyInfo& dep) const
{
	if (isTuneable)
		dep.VariesByFlags |= VariesBy_Tuneable;
}


// -----------------------------------------------------------------------------
// ------------------------------ FUNCTION EVALS -------------------------------
// -----------------------------------------------------------------------------
void EvaluateInt(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 1, "Int takes 1 param.");
	Result resX;
	args[0]->Evaluate(ec, resX);
	ExpectDim(n, 1, resX, "arg1");
	Convert(resX, VariableFormat::Int);
	res.Type = IntType;
	res.Value.IntVal = resX.Value.IntVal;
}

void EvaluateInt2(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 2, "Int2 takes 2 params.");
	Result resX, resY;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	Convert(resX, VariableFormat::Int);
	Convert(resY, VariableFormat::Int);
	res.Type = Int2Type;
	res.Value.Int4Val.x = resX.Value.IntVal;
	res.Value.Int4Val.y = resY.Value.IntVal;
}

void EvaluateInt3(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 3, "Int3 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
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

void EvaluateInt4(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 4, "Int4 takes 4 params.");
	Result resX, resY, resZ, resW;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
	args[3]->Evaluate(ec, resW);
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

void EvaluateUint(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 1, "Uint takes 1 param.");
	Result resX;
	args[0]->Evaluate(ec, resX);
	ExpectDim(n, 1, resX, "arg1");
	Convert(resX, VariableFormat::Uint);
	res.Type = UintType;
	res.Value.UintVal = resX.Value.UintVal;
}

void EvaluateUint2(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 2, "Uint2 takes 2 params.");
	Result resX, resY;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	Convert(resX, VariableFormat::Uint);
	Convert(resY, VariableFormat::Uint);
	res.Type = Uint2Type;
	res.Value.Uint4Val.x = resX.Value.UintVal;
	res.Value.Uint4Val.y = resY.Value.UintVal;
}

void EvaluateUint3(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 3, "Uint3 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
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

void EvaluateUint4(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 4, "Uint4 takes 4 params.");
	Result resX, resY, resZ, resW;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
	args[3]->Evaluate(ec, resW);
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

void EvaluateFloat(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 1, "Float takes 1 param.");
	Result resX;
	args[0]->Evaluate(ec, resX);
	ExpectDim(n, 1, resX, "arg1");
	Convert(resX, VariableFormat::Float);
	res.Type = FloatType;
	res.Value.FloatVal = resX.Value.FloatVal;
}

void EvaluateFloat2(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 2, "Float2 takes 2 params.");
	Result resX, resY;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	ExpectDim(n, 1, resX, "arg1");
	ExpectDim(n, 1, resY, "arg2");
	Convert(resX, VariableFormat::Float);
	Convert(resY, VariableFormat::Float);
	res.Type = Float2Type;
	res.Value.Float2Val.x = resX.Value.FloatVal;
	res.Value.Float2Val.y = resY.Value.FloatVal;
}

void EvaluateFloat3(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 3, "Float3 takes 3 params.");
	Result resX, resY, resZ;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
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

void EvaluateFloat4(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 4, "Float4 takes 4 params.");
	Result resX, resY, resZ, resW;
	args[0]->Evaluate(ec, resX);
	args[1]->Evaluate(ec, resY);
	args[2]->Evaluate(ec, resZ);
	args[3]->Evaluate(ec, resW);
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

void EvaluateSin(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 1, "Sin takes 1 param");
	Result argRes;
	args[0]->Evaluate(ec, argRes);
	Convert(argRes, VariableFormat::Float);
	res.Type = argRes.Type;
	for (u32 i = 0 ; i < argRes.Type.Dim ; ++i)
	{
		res.Value.Float4Val.m[i] = sin(argRes.Value.Float4Val.m[i]);
	}
}

void EvaluateCos(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 1, "Cos takes 1 param");
	Result argRes;
	args[0]->Evaluate(ec, argRes);
	Convert(argRes, VariableFormat::Float);
	res.Type = argRes.Type;
	for (u32 i = 0 ; i < argRes.Type.Dim ; ++i)
	{
		res.Value.Float4Val.m[i] = cos(argRes.Value.Float4Val.m[i]);
	}
}

void EvaluateLookAt(const Node* n, const EvaluationContext& ec, std::vector<Node*> args,
	Result& res)
{
	AstAssert(n, args.size() == 2, "LookAt takes 2 params");
	Result fromRes, toRes;
	args[0]->Evaluate(ec, fromRes);
	args[1]->Evaluate(ec, toRes);
	ExpectDim(n, 3, fromRes, "arg1 (from)");
	ExpectDim(n, 3, toRes, "arg2 (to)");
	Convert(fromRes, VariableFormat::Float);
	Convert(toRes, VariableFormat::Float);
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

typedef void (*FunctionEvaluate)(const Node*, const EvaluationContext&, std::vector<Node*>,
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
	{ LowerHash("LookAt"), { EvaluateLookAt, VariesBy_None} },
	{ LowerHash("Projection"), { EvaluateProjection, VariesBy_None} },
};

void Function::Evaluate(const EvaluationContext& ec, Result& res) const
{
	u32 funcHash = LowerHash(Name.c_str());
	AstAssert(this, FuncMap.count(funcHash) == 1, "No function named %s exists.",
		Name.c_str());
	FunctionEvaluate fi = FuncMap[funcHash].fn;
	fi(this, ec, Args, res);
}
void Function::GetDependency(DependencyInfo& dep) const
{
	u32 funcHash = LowerHash(Name.c_str());
	AstAssert(this, FuncMap.count(funcHash) == 1, "No function named %s exists.",
		Name.c_str());

	VariesBy vb = FuncMap[funcHash].vb;
	dep.VariesByFlags |= vb;

	for (Node* arg : Args)
		arg->GetDependency(dep);
}


#undef AstAssert


} // namespace ast
} // namespace rlf
