
namespace rlf
{

void EvaluateAstError(const ast::Node* n, const char* str, ...)
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

#define EvaluateAstAssert(node, expression, message, ...) 	\
do {												\
	if (!(expression)) {							\
		EvaluateAstError(node, message, ##__VA_ARGS__);		\
	}												\
} while (0);										\

void EvaluateExpression(ast::EvaluationContext& ec, ast::Node* ast, ast::Result& res)
{
	ErrorState es;
	ast::Evaluate(ec, ast, res, es);
	if (!es.Success)
	{
		ErrorInfo ee;
		ee.Location = es.Info.Location;
		ee.Message = "AST evaluation error: " + es.Info.Message;
		throw ee;
	}
}

void EvaluateExpression(ast::EvaluationContext& ec, ast::Node* ast, ast::Result& res, 
	VariableType expect, const char* name)
{
	EvaluateExpression(ec, ast, res);
	EvaluateAstAssert(ast,  (expect.Fmt != VariableFormat::Float4x4 && 
		res.Type.Fmt != VariableFormat::Float4x4) || expect.Fmt == res.Type.Fmt,
		"%s expected type (%s) is not compatible with actual type (%s)",
		name, TypeFmtToString(expect.Fmt), TypeFmtToString(res.Type.Fmt));
	EvaluateAstAssert(ast, expect.Dim == res.Type.Dim,
		"%s size (%u) does not match actual size (%u)",
		name, expect.Dim, res.Type.Dim);
	Convert(res, expect.Fmt);
}

void EvaluateConstants(ast::EvaluationContext& ec, std::vector<Constant*>& cnsts)
{
	for (Constant* cnst : cnsts)
	{
		ast::Result res;
		EvaluateExpression(ec, cnst->Expr, res, cnst->Type, cnst->Name);
		cnst->Value = res.Value;
	}
}

#undef EvaluateAstAssert


void InitD3D(
	gfx::Context* ctx,
	RenderDescription* rd,
	uint2 displaySize,
	const char* workingDirectory,
	ErrorState* errorState)
{
	errorState->Success = true;
	errorState->Warning = false;
	try {
		InitMain(ctx, rd, displaySize, workingDirectory, errorState);
	}
	catch (ErrorInfo ie)
	{
		errorState->Success = false;
		errorState->Info = ie;
	}
}

void Execute(
	ExecuteContext* ec,
	RenderDescription* rd,
	ErrorState* es)
{
	es->Success = true;
	try {
		_Execute(ec, rd);
	}
	catch (ErrorInfo ee)
	{
		es->Success = false;
		es->Info = ee;
	}
}

} // namespace rlf
