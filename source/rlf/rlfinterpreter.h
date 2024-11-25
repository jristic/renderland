
namespace rlf
{
	struct ExecuteResources
	{
		gfx::Texture*				MainRtTex;
		gfx::RenderTargetView		MainRtv;
		gfx::UnorderedAccessView	MainRtUav;
	};

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
		ExecuteResources Res;
		ast::EvaluationContext EvCtx;
	};


	void EvaluateExpression(ast::EvaluationContext& ec, ast::Expression& expr, ast::Result& res);
	void EvaluateExpression(ast::EvaluationContext& ec, ast::Expression& expr, ast::Result& res, 
		VariableType expect, const char* name);
	void EvaluateConstants(ast::EvaluationContext& ec, Array<Constant*> cnsts);

	void GenerateTextureResource(const char* texMem, u32 memSize, const char* ext, 
		DirectX::ScratchImage* out);

	void HandleTextureParametersChanged(
		RenderDescription* rd,
		ExecuteContext* ec,
		ErrorState* errorState);

	void Execute(
		ExecuteContext* context,
		RenderDescription* rd,
		ErrorState* errorState);
}
