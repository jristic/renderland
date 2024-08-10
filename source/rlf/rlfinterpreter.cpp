
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

void EvaluateExpression(ast::EvaluationContext& ec, ast::Expression& expr, ast::Result& res)
{
	ErrorState es;
	ast::Evaluate(ec, expr, res, es);
	if (!es.Success)
	{
		ErrorInfo ee;
		ee.Location = es.Info.Location;
		ee.Message = "AST evaluation error: " + es.Info.Message;
		throw ee;
	}
}

void EvaluateExpression(ast::EvaluationContext& ec, ast::Expression& expr, ast::Result& res, 
	VariableType expect, const char* name)
{
	const ast::Node* ast = expr.TopNode;
	EvaluateExpression(ec, expr, res);
	EvaluateAstAssert(ast,  (expect.Fmt != VariableFormat::Float4x4 && 
		res.Type.Fmt != VariableFormat::Float4x4) || expect.Fmt == res.Type.Fmt,
		"%s expected type (%s) is not compatible with actual type (%s)",
		name, TypeFmtToString(expect.Fmt), TypeFmtToString(res.Type.Fmt));
	EvaluateAstAssert(ast, expect.Dim == res.Type.Dim,
		"%s size (%u) does not match actual size (%u)",
		name, expect.Dim, res.Type.Dim);
	Convert(res, expect.Fmt);
}

void EvaluateConstants(ast::EvaluationContext& ec, Array<Constant*> cnsts)
{
	for (Constant* cnst : cnsts)
	{
		ast::Result res;
		EvaluateExpression(ec, cnst->Expr, res, cnst->Type, cnst->Name);
		cnst->Value = res.Value;
	}
}

bool IsCompressedFormat(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return true;
	default:
		return false;
	}
}

void GenerateTextureResource(const char* texMem, u32 memSize, const char* ext, 
	DirectX::ScratchImage* out)
{
	DirectX::TexMetadata origMeta = {};
	DirectX::ScratchImage orig;
	if (strcmp(ext, "dds") == 0)
		DirectX::LoadFromDDSMemory(texMem, memSize, DirectX::DDS_FLAGS_NONE, 
			&origMeta, orig);
	else if (strcmp(ext, "tga") == 0)
		DirectX::LoadFromTGAMemory(texMem, memSize, DirectX::TGA_FLAGS_NONE, 
			&origMeta, orig);
	else
		InitError("Unsupported Texture::FromFile extension (%s)", ext);

	bool is_compressed = IsCompressedFormat(origMeta.format);

	DirectX::ScratchImage decompressed;
	if (is_compressed)
	{
		HRESULT hr = DirectX::Decompress(orig.GetImages(), orig.GetImageCount(), 
			origMeta, DXGI_FORMAT_UNKNOWN, decompressed);
		Assert(hr == S_OK, "Failed to decompress, hr=%x", hr);
	}

	DirectX::ScratchImage* toMip = is_compressed ? &decompressed : &orig;
	DirectX::ScratchImage mipped;
	HRESULT hr = DirectX::GenerateMipMaps( toMip->GetImages(), toMip->GetImageCount(),
		toMip->GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, 
		is_compressed ? mipped : *out );
	Assert(hr == S_OK, "Failed to create mips, hr=%x", hr);

	if (is_compressed)
	{
		hr = DirectX::Compress(mipped.GetImages(), mipped.GetImageCount(), 
			mipped.GetMetadata(),origMeta.format, DirectX::TEX_COMPRESS_DEFAULT, 
			DirectX::TEX_THRESHOLD_DEFAULT, *out);
		Assert(hr == S_OK, "Failed to recompress, hr=%x", hr);
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
