
namespace rlf
{
	void InitD3D(
		gfx::Context* ctx,
		RenderDescription* rd,
		uint2 displaySize,
		const char* workingDirectory,
		ErrorState* errorState);

	void ReleaseD3D(
		gfx::Context* ctx,
		RenderDescription* rd);

	struct ExecuteContext
	{
		gfx::Context* GfxCtx;
		gfx::RenderTargetView MainRtv;
		gfx::UnorderedAccessView MainRtUav;
		gfx::DepthStencilView DefaultDepthView;
		ast::EvaluationContext EvCtx;
	};


	void EvaluateExpression(ast::EvaluationContext& ec, ast::Node* ast, ast::Result& res);
	void EvaluateExpression(ast::EvaluationContext& ec, ast::Node* ast, ast::Result& res, 
		VariableType expect, const char* name);
	void EvaluateConstants(ast::EvaluationContext& ec, std::vector<Constant*>& cnsts);


	void HandleTextureParametersChanged(
		RenderDescription* rd,
		ExecuteContext* ec,
		ErrorState* errorState);

	void Execute(
		ExecuteContext* context,
		RenderDescription* rd,
		ErrorState* errorState);
}
