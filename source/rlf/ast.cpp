
namespace rlf {
namespace ast {


void FloatLiteral::Evaluate(const EvaluationContext& , Result *res)
{
	res->Type = Result::Type::Float;
	res->FloatVal = Val;
}

void Float3::Evaluate(const EvaluationContext& ec, Result *res)
{
	Result resX, resY, resZ;
	X->Evaluate(ec, &resX);
	Y->Evaluate(ec, &resY);
	Z->Evaluate(ec, &resZ);
	Assert(resX.Type == Result::Type::Float && resY.Type == Result::Type::Float && 
		resZ.Type == Result::Type::Float, "invalid");
	res->Type = Result::Type::Float3;
	res->Float3Val.x = resX.FloatVal;
	res->Float3Val.y = resY.FloatVal;
	res->Float3Val.z = resZ.FloatVal;
}

void Subscript::Evaluate(const EvaluationContext& ec, Result *res)
{
	Result subjectRes;
	Subject->Evaluate(ec, &subjectRes);
	Assert(subjectRes.Type == Result::Type::Float2, "not handled"); // TODO: expand
	res->Type = Result::Type::Float;
	if (Index == 0)
		res->FloatVal = subjectRes.Float2Val.x;
	else if (Index == 1)
		res->FloatVal = subjectRes.Float2Val.y;
	else
		Unimplemented();
}

void Multiply::Evaluate(const EvaluationContext& ec, Result *res)
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, &arg1Res);
	Arg2->Evaluate(ec, &arg2Res);
	Assert(arg1Res.Type == Result::Type::Float4x4 && 
		arg2Res.Type == Result::Type::Float4x4, "invalid");
	res->Type = Result::Type::Float4x4;
	res->Float4x4Val = arg1Res.Float4x4Val * arg2Res.Float4x4Val;
}

void Divide::Evaluate(const EvaluationContext& ec, Result *res)
{
	Result arg1Res, arg2Res;
	Arg1->Evaluate(ec, &arg1Res);
	Arg2->Evaluate(ec, &arg2Res);
	Assert(arg1Res.Type == Result::Type::Float && arg2Res.Type == Result::Type::Float,
		"invalid");
	res->Type = Result::Type::Float;
	res->FloatVal = arg1Res.FloatVal / arg2Res.FloatVal;
}

void Time::Evaluate(const EvaluationContext& ec, Result *res)
{
	res->Type = Result::Type::Float;
	res->FloatVal = ec.Time;
}

void DisplaySize::Evaluate(const EvaluationContext& ec, Result *res)
{
	res->Type = Result::Type::Float2;
	res->Float2Val.x = (float)ec.DisplaySize.x;
	res->Float2Val.y = (float)ec.DisplaySize.y;
}

void LookAt::Evaluate(const EvaluationContext& ec, Result *res)
{
	Result fromRes, toRes;
	From->Evaluate(ec, &fromRes);
	To->Evaluate(ec, &toRes);
	Assert(fromRes.Type == Result::Type::Float3 && toRes.Type == Result::Type::Float3,
		"invalid");
	res->Type = Result::Type::Float4x4;
	res->Float4x4Val = lookAt(fromRes.Float3Val, toRes.Float3Val);
}

void Projection::Evaluate(const EvaluationContext& ec, Result *res)
{
	Result fovRes, aspectRes, nearRes, farRes;
	Fov->Evaluate(ec, &fovRes);
	Aspect->Evaluate(ec, &aspectRes);
	ZNear->Evaluate(ec, &nearRes);
	ZFar->Evaluate(ec, &farRes);
	Assert(fovRes.Type == Result::Type::Float && aspectRes.Type == Result::Type::Float &&
		nearRes.Type == Result::Type::Float && farRes.Type == Result::Type::Float,
		"invalid");
	res->Type = Result::Type::Float4x4;
	res->Float4x4Val = projection(fovRes.FloatVal, aspectRes.FloatVal, 
		nearRes.FloatVal, farRes.FloatVal);
}


}
}
