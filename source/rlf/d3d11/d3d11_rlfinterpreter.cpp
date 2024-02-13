
namespace rlf
{

#define SafeRelease(ref) do { if (ref) { (ref)->Release(); (ref) = nullptr; } } while (0);


// -----------------------------------------------------------------------------
// ------------------------------ INITIALIZATION -------------------------------
// -----------------------------------------------------------------------------
void InitError(const char* str, ...)
{
	char buf[2048];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,2048,str,ptr);
	va_end(ptr);

	ErrorInfo ie;
	ie.Location = nullptr;
	ie.Message = buf;
	throw ie;
}
void InitErrorEx(const char* message)
{
	ErrorInfo ie;
	ie.Location = nullptr;
	ie.Message = message;
	throw ie;
}

static ID3D11InfoQueue*	gInfoQueue = nullptr;

void CheckHresult(HRESULT hr, const char* desc)
{
	if (hr == S_OK)
		return;
	std::string mes = "Failed to create ";
	mes += desc;
	mes += ". See D3D error message:\n";
	UINT64 num = gInfoQueue->GetNumStoredMessages();
	for (u32 i = 0 ; i < num ; ++i)
	{
		size_t messageLength;
		HRESULT hrr = gInfoQueue->GetMessage(i, nullptr, &messageLength);
		Assert(hrr == S_FALSE, "Failed to get message, hr=%x", hr);
		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageLength);
		gInfoQueue->GetMessage(i, message, &messageLength);
		mes += message->pDescription;
		mes += "\n";
		free(message);
	}
	gInfoQueue->ClearStoredMessages();

	ErrorInfo ie;
	ie.Location = nullptr;
	ie.Message = mes.c_str();
	throw ie;
}

#define InitAssert(expression, message, ...) \
do {										\
	if (!(expression)) {					\
		InitError(message, ##__VA_ARGS__);	\
	}										\
} while (0);								\

static ID3D11RasterizerState*   DefaultRasterizerState = nullptr;

D3D11_FILTER RlfToD3d(FilterMode fm)
{
	if (fm.Min == Filter::Aniso || fm.Mag == Filter::Aniso ||
		fm.Mip == Filter::Aniso)
	{
		return D3D11_FILTER_ANISOTROPIC;
	}

	static D3D11_FILTER filters[] = {
		D3D11_FILTER_MIN_MAG_MIP_POINT,
		D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
		D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
		D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
		D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
		D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_FILTER_MIN_MAG_MIP_LINEAR,
	};

	u32 i = 0;
	if (fm.Min == Filter::Linear)
		i += 4;
	if (fm.Mag == Filter::Linear)
		i += 2;
	if (fm.Mip == Filter::Linear)
		i += 1;
	
	return filters[i];
}

D3D11_TEXTURE_ADDRESS_MODE RlfToD3d(AddressMode m)
{
	Assert(m != AddressMode::Invalid, "Invalid");
	static D3D11_TEXTURE_ADDRESS_MODE modes[] = {
		(D3D11_TEXTURE_ADDRESS_MODE)0, //invalid
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_MIRROR,
		D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_BORDER,
	};
	return modes[(u32)m];
}

D3D11_PRIMITIVE_TOPOLOGY RlfToD3d(Topology topo)
{
	Assert(topo != Topology::Invalid, "Invalid");
	static D3D11_PRIMITIVE_TOPOLOGY topos[] = {
		(D3D11_PRIMITIVE_TOPOLOGY)0, //invalid
		D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	};
	return topos[(u32)topo];
}

D3D11_CULL_MODE RlfToD3d(CullMode cm)
{
	Assert(cm != CullMode::Invalid, "Invalid");
	static D3D11_CULL_MODE cms[] = {
		(D3D11_CULL_MODE)0, //invalid
		D3D11_CULL_NONE,
		D3D11_CULL_FRONT,
		D3D11_CULL_BACK,
	};
	return cms[(u32)cm];
}

u32 RlfToD3d_Bind(BufferFlag flags)
{
	u32 f = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	if (flags & BufferFlag_Vertex)
		f |= D3D11_BIND_VERTEX_BUFFER;
	if (flags & BufferFlag_Index)
		f |= D3D11_BIND_INDEX_BUFFER;
	return f;
}

u32 RlfToD3d_Misc(BufferFlag flags)
{
	u32 f = 0;
	if (flags & BufferFlag_Structured) 
		f |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	if (flags & BufferFlag_IndirectArgs) 
		f |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	if (flags & BufferFlag_Raw)
		f |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	return f;
}

u32 RlfToD3d(TextureFlag flags)
{
	u32 f = 0;
	if (flags & TextureFlag_SRV)
		f |= D3D11_BIND_SHADER_RESOURCE; 
	if (flags & TextureFlag_UAV)
		f |= D3D11_BIND_UNORDERED_ACCESS;
	if (flags & TextureFlag_RTV)
		f |= D3D11_BIND_RENDER_TARGET;
	if (flags & TextureFlag_DSV)
		f |= D3D11_BIND_DEPTH_STENCIL;
	return f;
}

D3D11_COMPARISON_FUNC RlfToD3d(ComparisonFunc cf)
{
	Assert(cf != ComparisonFunc::Invalid, "Invalid");
	static D3D11_COMPARISON_FUNC cfs[] = {
		(D3D11_COMPARISON_FUNC)0, //invalid
		D3D11_COMPARISON_NEVER,
		D3D11_COMPARISON_LESS,
		D3D11_COMPARISON_EQUAL,
		D3D11_COMPARISON_LESS_EQUAL,
		D3D11_COMPARISON_GREATER,
		D3D11_COMPARISON_NOT_EQUAL,
		D3D11_COMPARISON_GREATER_EQUAL,
		D3D11_COMPARISON_ALWAYS,
	};
	return cfs[(u32)cf];
}

D3D11_STENCIL_OP RlfToD3d(StencilOp so)
{
	Assert(so != StencilOp::Invalid, "Invalid");
	static D3D11_STENCIL_OP sos[] = {
		(D3D11_STENCIL_OP)0, //invalid
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_ZERO,
		D3D11_STENCIL_OP_REPLACE,
		D3D11_STENCIL_OP_INCR_SAT,
		D3D11_STENCIL_OP_DECR_SAT,
		D3D11_STENCIL_OP_INVERT,
		D3D11_STENCIL_OP_INCR,
		D3D11_STENCIL_OP_DECR
	};
	return sos[(u32)so];
}

D3D11_DEPTH_STENCILOP_DESC RlfToD3d(StencilOpDesc sod)
{
	D3D11_DEPTH_STENCILOP_DESC desc = {};
	desc.StencilFailOp = RlfToD3d(sod.StencilFailOp);
	desc.StencilDepthFailOp = RlfToD3d(sod.StencilDepthFailOp);
	desc.StencilPassOp = RlfToD3d(sod.StencilPassOp);
	desc.StencilFunc = RlfToD3d(sod.StencilFunc);
	return desc;
}

D3D11_BLEND RlfToD3d(Blend b)
{
	Assert(b != Blend::Invalid, "Invalid");
	static D3D11_BLEND bs[] = {
		(D3D11_BLEND)0, //invalid
		D3D11_BLEND_ZERO,
		D3D11_BLEND_ONE,
		D3D11_BLEND_SRC_COLOR,
		D3D11_BLEND_INV_SRC_COLOR,
		D3D11_BLEND_SRC_ALPHA,
		D3D11_BLEND_INV_SRC_ALPHA,
		D3D11_BLEND_DEST_ALPHA,
		D3D11_BLEND_INV_DEST_ALPHA,
		D3D11_BLEND_DEST_COLOR,
		D3D11_BLEND_INV_DEST_COLOR,
		D3D11_BLEND_SRC_ALPHA_SAT,
		D3D11_BLEND_BLEND_FACTOR,
		D3D11_BLEND_INV_BLEND_FACTOR,
		D3D11_BLEND_SRC1_COLOR,
		D3D11_BLEND_INV_SRC1_COLOR,
		D3D11_BLEND_SRC1_ALPHA,
		D3D11_BLEND_INV_SRC1_ALPHA,
	};
	return bs[(u32)b];
}

D3D11_BLEND_OP RlfToD3d(BlendOp op)
{
	Assert(op != BlendOp::Invalid, "Invalid");
	static D3D11_BLEND_OP ops[] = {
		(D3D11_BLEND_OP)0, //invalid
		D3D11_BLEND_OP_ADD,
		D3D11_BLEND_OP_SUBTRACT,
		D3D11_BLEND_OP_REV_SUBTRACT,
		D3D11_BLEND_OP_MIN,
		D3D11_BLEND_OP_MAX,
	};
	return ops[(u32)op];
}

D3D11_INPUT_CLASSIFICATION RlfToD3d(InputClassification ic)
{
	Assert(ic != InputClassification::Invalid, "Invalid");
	static D3D11_INPUT_CLASSIFICATION ics[] = {
		(D3D11_INPUT_CLASSIFICATION)0, //invalid
		D3D11_INPUT_PER_VERTEX_DATA ,
		D3D11_INPUT_PER_INSTANCE_DATA,
	};
	return ics[(u32)ic];
}

ID3DBlob* CommonCompileShader(CommonShader* common, const char* dirPath, const char* profile, 
	ErrorState* errorState)
{
	std::string shaderPath = std::string(dirPath) + common->ShaderPath;
	const char* path = shaderPath.c_str();
	HANDLE shader = fileio::OpenFileOptional(path, GENERIC_READ);
	InitAssert(shader != INVALID_HANDLE_VALUE, "Couldn't find shader file: %s", path);

	u32 shaderSize = fileio::GetFileSize(shader);

	char* shaderBuffer = (char*)malloc(shaderSize);
	Assert(shaderBuffer != nullptr, "failed to alloc");

	fileio::ReadFile(shader, shaderBuffer, shaderSize);

	CloseHandle(shader);

	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	HRESULT hr = D3DCompile(shaderBuffer, shaderSize, path, NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, common->EntryPoint, profile, D3DCOMPILE_DEBUG, 
		0, &shaderBlob, &errorBlob);

	bool success = (hr == S_OK);

	if (!success)
	{
		Assert(errorBlob != nullptr, "No error info given for shader compile fail.");
		char* errorText = (char*)errorBlob->GetBufferPointer();
		std::string textCopy = errorText;

		Assert(shaderBlob == nullptr, "leak");
		SafeRelease(errorBlob);
		free(shaderBuffer);

		InitErrorEx(("Failed to compile shader:\n " + textCopy).c_str());
	}

	// check for warnings
	if (errorBlob)
	{
		errorState->Warning = true;
		char* errorText = (char*)errorBlob->GetBufferPointer();
		errorState->Info.Location = nullptr; // file&line already provided in text.
		errorState->Info.Message += errorText;
	}
	SafeRelease(errorBlob);
	
	if (common->SizeRequests.size() > 0)
	{
		std::unordered_map<std::string, u32> structSizes;
		ErrorState es;
		shader::ParseBuffer(shaderBuffer, shaderSize, structSizes, &es);
		if (!es.Success)
		{
			free(shaderBuffer);
			InitError("Failed to parse shader:\n%s", es.Info.Message.c_str());
		}
		for (ast::SizeOf* request : common->SizeRequests)
		{
			auto search = structSizes.find(request->StructName);
			InitAssert(search != structSizes.end(), 
				"Failed to parse shader: %s\nNo struct named %s found for sizeof operation",
				path, request->StructName.c_str());
			request->Size = search->second;
		}
	}

	free(shaderBuffer);

	return shaderBlob;
}

void CreateInputLayout(ID3D11Device* device, VertexShader* shader, ID3DBlob* blob)
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	if (shader->InputLayout.empty())
	{
		// Read input layout description from shader info
		ID3D11ShaderReflection* reflector = shader->Common.Reflector;

		// Get shader info
		D3D11_SHADER_DESC shaderDesc;
		reflector->GetDesc( &shaderDesc );

		for ( u32 i = 0 ; i < shaderDesc.InputParameters ; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			reflector->GetInputParameterDesc(i, &paramDesc );

			if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED)
				continue;

			// fill out input element desc
			D3D11_INPUT_ELEMENT_DESC elementDesc;
			elementDesc.SemanticName = paramDesc.SemanticName;
			elementDesc.SemanticIndex = paramDesc.SemanticIndex;
			elementDesc.InputSlot = 0;
			elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			elementDesc.InstanceDataStepRate = 0;   

			// determine DXGI format
			if ( paramDesc.Mask == 1 )
			{
				if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
					elementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
					elementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
					elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if ( paramDesc.Mask <= 3 )
			{
				if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if ( paramDesc.Mask <= 7 )
			{
				if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if ( paramDesc.Mask <= 15 )
			{
				if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
					elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			//save element desc
			inputLayoutDesc.push_back(elementDesc);
		}
	}
	else
	{
		inputLayoutDesc.resize(shader->InputLayout.size());
		for (u32 i = 0 ; i < shader->InputLayout.size() ; ++i)
		{
			D3D11_INPUT_ELEMENT_DESC& out = inputLayoutDesc[i];
			InputElementDesc& in = shader->InputLayout[i];
			out.SemanticName = in.SemanticName;
			out.SemanticIndex = in.SemanticIndex;
			out.Format = D3DTextureFormat[(u32)in.Format];
			out.InputSlot = in.InputSlot;
			out.AlignedByteOffset = in.AlignedByteOffset;
			out.InputSlotClass = RlfToD3d(in.InputSlotClass);
			out.InstanceDataStepRate = in.InstanceDataStepRate;
		}
	}

	if (inputLayoutDesc.size())
	{
		// Try to create Input Layout
		HRESULT hr = device->CreateInputLayout(
			&inputLayoutDesc[0], (u32)inputLayoutDesc.size(), blob->GetBufferPointer(), 
			blob->GetBufferSize(), &shader->LayoutGfxState);
		Assert(hr == S_OK, "failed to create input layout, hr=%x", hr);
	}
}

void CreateTexture(ID3D11Device* device, Texture* tex)
{
	Assert(tex->GfxState == nullptr, "Leaking object");

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = tex->Size.x;
	desc.Height = tex->Size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = D3DTextureFormat[(u32)tex->Format];
	desc.SampleDesc.Count = tex->SampleCount;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = RlfToD3d(tex->Flags);
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	HRESULT hr = device->CreateTexture2D(&desc, nullptr, &tex->GfxState);
	CheckHresult(hr, "Texture");
}

void CreateBuffer(ID3D11Device* device, Buffer* buf)
{
	u32 bufSize = buf->ElementSize * buf->ElementCount;
	void* initData = malloc(bufSize);
	if (buf->InitToZero)
		ZeroMemory(initData, bufSize);
	else if (buf->InitDataSize > 0)
	{
		Assert(buf->InitData, "No data but size is set");
		memcpy(initData, buf->InitData, buf->InitDataSize);
	}

	D3D11_SUBRESOURCE_DATA subRes = {};
	subRes.pSysMem = initData;

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = bufSize;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = RlfToD3d_Bind(buf->Flags);
	desc.CPUAccessFlags = 0;
	desc.StructureByteStride = buf->ElementSize;
	desc.MiscFlags = RlfToD3d_Misc(buf->Flags); 

	HRESULT hr = device->CreateBuffer(&desc, subRes.pSysMem ? &subRes : nullptr,
		&buf->GfxState);
	free(initData);
	CheckHresult(hr, "Buffer");
}

void CreateView(ID3D11Device* device, View* v)
{
	ID3D11Resource* res = nullptr;
	if (v->ResourceType == ResourceType::Buffer)
		res = v->Buffer->GfxState;
	else if (v->ResourceType == ResourceType::Texture)
		res = v->Texture->GfxState;
	else
		Unimplemented();
	DXGI_FORMAT fmt = D3DTextureFormat[(u32)v->Format];
	if (v->Type == ViewType::SRV)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC vd;
		vd.Format = fmt;
		if (v->ResourceType == ResourceType::Buffer)
		{
			if (v->Buffer->Flags & BufferFlag_Raw)
			{
				vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
				vd.BufferEx.FirstElement = 0;
				vd.BufferEx.NumElements = v->NumElements > 0 ? v->NumElements :
					v->Buffer->ElementCount;
				vd.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			}
			else
			{
				vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				vd.Buffer.FirstElement = 0;
				vd.Buffer.NumElements = v->NumElements > 0 ? v->NumElements :
					v->Buffer->ElementCount;
			}
		}
		else if (v->ResourceType == ResourceType::Texture)
		{
			vd.ViewDimension = v->Texture->SampleCount > 1 ? 
				D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
			vd.Texture2D.MostDetailedMip = 0;
			vd.Texture2D.MipLevels = (u32)-1;
		}
		else 
			Unimplemented();
		HRESULT hr = device->CreateShaderResourceView(res, &vd, &v->SRVGfxState);
		CheckHresult(hr, "SRV");
	}
	else if (v->Type == ViewType::UAV)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC vd;
		vd.Format = fmt;
		if (v->ResourceType == ResourceType::Buffer)
		{
			vd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			vd.Buffer.FirstElement = 0;
			vd.Buffer.NumElements = v->NumElements > 0 ? v->NumElements : 
				v->Buffer->ElementCount;
			vd.Buffer.Flags = v->Buffer->Flags & BufferFlag_Raw ? 
				D3D11_BUFFER_UAV_FLAG_RAW : 0;
		}
		else if (v->ResourceType == ResourceType::Texture)
		{
			vd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			vd.Texture2D.MipSlice = 0;
		}
		else 
			Unimplemented();
		HRESULT hr = device->CreateUnorderedAccessView(res, &vd, &v->UAVGfxState);
		CheckHresult(hr, "UAV");
	}
	else if (v->Type == ViewType::RTV)
	{
		D3D11_RENDER_TARGET_VIEW_DESC vd;
		vd.Format = fmt;
		Assert(v->ResourceType == ResourceType::Texture, "Invalid");
		vd.ViewDimension = v->Texture->SampleCount > 1 ? 
			D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
		vd.Texture2D.MipSlice = 0;
		HRESULT hr = device->CreateRenderTargetView(res, &vd, &v->RTVGfxState);
		CheckHresult(hr, "RTV");
	}
	else if (v->Type == ViewType::DSV)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC vd;
		vd.Format = fmt;
		Assert(v->ResourceType == ResourceType::Texture, "Invalid");
		vd.ViewDimension = v->Texture->SampleCount > 1 ? 
			D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		vd.Texture2D.MipSlice = 0;
		vd.Flags = 0;
		HRESULT hr = device->CreateDepthStencilView(res, &vd, &v->DSVGfxState);
		CheckHresult(hr, "DSV");
	}
	else
		Unimplemented();
}

void ReleaseView(View* v)
{
	switch (v->Type)
	{
	case ViewType::SRV:
		SafeRelease(v->SRVGfxState);
		break;
	case ViewType::UAV:
		SafeRelease(v->UAVGfxState);
		break;
	case ViewType::RTV:
		SafeRelease(v->RTVGfxState);
		break;
	case ViewType::DSV:
		SafeRelease(v->DSVGfxState);
		break;
	case ViewType::Auto:
		// NOTE: intentional do-nothing, if Auto hasn't been replaced then an object
		//	was never created.
		break;
	default:
		Unimplemented();
	}
}

void ResolveBind(Bind& bind, ID3D11ShaderReflection* reflector, const char* path)
{
	D3D11_SHADER_INPUT_BIND_DESC desc;
	HRESULT hr = reflector->GetResourceBindingDescByName(bind.BindTarget, 
		&desc);
	Assert(hr == S_OK || hr == E_INVALIDARG, "unhandled error, hr=%x", hr);
	InitAssert(hr != E_INVALIDARG, "Couldn't find resource %s in shader %s", 
		bind.BindTarget, path);
	bind.BindIndex = desc.BindPoint;
	switch (desc.Type)
	{
	case D3D_SIT_TEXTURE:
	case D3D_SIT_STRUCTURED:
	case D3D_SIT_BYTEADDRESS:
		InitAssert(bind.Type == BindType::SystemValue || bind.Type == BindType::View && 
			(bind.ViewBind->Type == ViewType::Auto || bind.ViewBind->Type == ViewType::SRV),
			"Mismatched bind to SRV slot.")
		bind.IsOutput = false;
		break;
	case D3D_SIT_SAMPLER:
		InitAssert(bind.Type == BindType::Sampler, "Mismatched bind to Sampler slot.")
		bind.IsOutput = false;
		break;
	case D3D_SIT_UAV_RWTYPED:
	case D3D_SIT_UAV_RWSTRUCTURED:
	case D3D_SIT_UAV_RWBYTEADDRESS:
		InitAssert(bind.Type == BindType::SystemValue || bind.Type == BindType::View && 
			(bind.ViewBind->Type == ViewType::Auto || bind.ViewBind->Type == ViewType::UAV),
			"Mismatched bind to UAV slot.")
		bind.IsOutput = true;
		break;
	case D3D_SIT_UAV_APPEND_STRUCTURED:
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
	case D3D_SIT_CBUFFER:
	case D3D_SIT_TBUFFER:
	default:
		Assert(false, "Unhandled type %d", desc.Type);
	}
	if (bind.Type == BindType::View && bind.ViewBind->Type == ViewType::Auto)
	{
		if (bind.IsOutput)
			bind.ViewBind->Type = ViewType::UAV;
		else
			bind.ViewBind->Type = ViewType::SRV;
	}
}

void PrepareConstants(
	ID3D11Device* device, ID3D11ShaderReflection* reflector, 
	std::vector<ConstantBuffer>& buffers, std::vector<SetConstant>& sets, 
	const char* path)
{
	(void)path;
	D3D11_SHADER_DESC sd;
	reflector->GetDesc(&sd);
	for (u32 i = 0 ; i < sd.ConstantBuffers ; ++i)
	{
		ConstantBuffer cb = {};
		D3D11_SHADER_BUFFER_DESC bd;
		ID3D11ShaderReflectionConstantBuffer* constBuffer = 
			reflector->GetConstantBufferByIndex(i);
		constBuffer->GetDesc(&bd);

		if (bd.Type != D3D11_CT_CBUFFER)
			continue;

		cb.Name = bd.Name;
		cb.Size = bd.Size;
		cb.BackingMemory = (u8*)malloc(bd.Size);

		for (u32 j = 0 ; j < bd.Variables ; ++j)
		{
			ID3D11ShaderReflectionVariable* var = constBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC vd;
			var->GetDesc(&vd);

			if (vd.DefaultValue)
				memcpy(cb.BackingMemory+vd.StartOffset, vd.DefaultValue, vd.Size);
			else
				ZeroMemory(cb.BackingMemory+vd.StartOffset, vd.Size);
		}

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = bd.Size;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = cb.BackingMemory;
		HRESULT hr = device->CreateBuffer(&desc, &srd, &cb.GfxState);
		Assert(hr == S_OK, "Failed to create CB hr=%x", hr);

		for (u32 k = 0 ; k < sd.BoundResources ; ++k)
		{
			D3D11_SHADER_INPUT_BIND_DESC id; 
			hr = reflector->GetResourceBindingDesc(k, &id);
			Assert(hr == S_OK, "Failed to get desc, hr=%x", hr);
			if (strcmp(id.Name, bd.Name) == 0)
			{
				cb.Slot = id.BindPoint;
				break;
			}
		}

		buffers.push_back(cb);
	}

	for (SetConstant& set : sets)
	{
		ID3D11ShaderReflectionVariable* cvar = nullptr;
		const char* cbufname = nullptr;
		for (u32 i = 0 ; i < sd.ConstantBuffers ; ++i)
		{
			D3D11_SHADER_BUFFER_DESC bd;
			ID3D11ShaderReflectionConstantBuffer* buffer = 
				reflector->GetConstantBufferByIndex(i);
			buffer->GetDesc(&bd);
			if (bd.Type != D3D11_CT_CBUFFER)
				continue;
			for (u32 j = 0 ; j < bd.Variables ; ++j)
			{
				ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(j);
				D3D11_SHADER_VARIABLE_DESC vd;
				var->GetDesc(&vd);
				if (strcmp(set.VariableName, vd.Name) == 0)
				{
					cvar = var;
					cbufname = bd.Name;
					break;
				}
			}
		}
		InitAssert(cvar, "Couldn't find constant %s in shader %s", set.VariableName, path);
		D3D11_SHADER_VARIABLE_DESC vd;
		cvar->GetDesc(&vd);
		set.Offset = vd.StartOffset;
		set.Size = vd.Size;
		bool found = false;
		for (ConstantBuffer& con : buffers)
		{
			if (con.Name == cbufname)
			{
				set.CB = &con;
				found = true;
				break;
			}
		}
		Assert(found, "Failed to find cbuffer resource binding");

		ID3D11ShaderReflectionType* ctype = cvar->GetType();
		D3D11_SHADER_TYPE_DESC td;
		ctype->GetDesc(&td);
		switch (td.Type)
		{
		case D3D_SVT_BOOL:
			Assert(td.Rows == 1, "Unhandled bool vector");
			set.Type.Fmt = VariableFormat::Bool;
			set.Type.Dim = td.Columns;
			break;
		case D3D_SVT_INT:
			Assert(td.Rows == 1, "Unhandled int vector");
			set.Type.Fmt = VariableFormat::Int;
			set.Type.Dim = td.Columns;
			break;
		case D3D_SVT_UINT:
			Assert(td.Rows == 1, "Unhandled uint vector");
			set.Type.Fmt = VariableFormat::Uint;
			set.Type.Dim = td.Columns;
			break;
		case D3D_SVT_FLOAT:
			if (td.Rows == 4 && td.Columns == 4)
			{
				set.Type = Float4x4Type;
			}
			else
			{
				Assert(td.Rows == 1, "Unhandled float vector");
				set.Type.Fmt = VariableFormat::Float;
				set.Type.Dim = td.Columns;
			}
			break;
		default:
			Unimplemented();
		}
	}
}

void InitMain(
	gfx::Context* ctx,
	ExecuteResources*, // param intentionally unused in this backend
	RenderDescription* rd,
	uint2 displaySize,
	const char* workingDirectory,
	ErrorState* errorState)
{
	ID3D11Device* device = ctx->Device;
	gInfoQueue = ctx->InfoQueue;
	
	if (DefaultRasterizerState == nullptr)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		device->CreateRasterizerState(&desc, &DefaultRasterizerState);
	}
	
	std::string dirPath = workingDirectory;

	for (ComputeShader* cs : rd->CShaders)
	{
		ID3DBlob* shaderBlob = CommonCompileShader(&cs->Common, workingDirectory,
			"cs_5_0", errorState);

		HRESULT hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &cs->GfxState);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &cs->Common.Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		cs->Common.Reflector->GetThreadGroupSize(&cs->ThreadGroupSize.x, 
			&cs->ThreadGroupSize.y,	&cs->ThreadGroupSize.z);

		SafeRelease(shaderBlob);
	}

	for (VertexShader* vs : rd->VShaders)
	{
		ID3DBlob* shaderBlob = CommonCompileShader(&vs->Common, workingDirectory,
			"vs_5_0", errorState);

		HRESULT hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &vs->GfxState);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &vs->Common.Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		CreateInputLayout(device, vs, shaderBlob);

		SafeRelease(shaderBlob);
	}

	for (PixelShader* ps : rd->PShaders)
	{
		ID3DBlob* shaderBlob = CommonCompileShader(&ps->Common, workingDirectory,
			"ps_5_0", errorState);

		HRESULT hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &ps->GfxState);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &ps->Common.Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		SafeRelease(shaderBlob);
	}


	for (Dispatch* dc : rd->Dispatches)
	{
		ID3D11ShaderReflection* reflector = dc->Shader->Common.Reflector;
		for (Bind& bind : dc->Binds)
		{
			ResolveBind(bind, reflector, dc->Shader->Common.ShaderPath);
		}
		PrepareConstants(device, reflector, dc->CBs, dc->Constants,
			dc->Shader->Common.ShaderPath);
	}

	for (Draw* draw : rd->Draws)
	{
		InitAssert(draw->VShader, "Null vertex shader on draw not permitted.");
		ID3D11ShaderReflection* reflector = draw->VShader->Common.Reflector;
		for (Bind& bind : draw->VSBinds)
		{
			ResolveBind(bind, reflector, draw->VShader->Common.ShaderPath);
		}
		PrepareConstants(device, reflector, draw->VSCBs, draw->VSConstants,
			draw->VShader->Common.ShaderPath);
		if (draw->PShader)
		{
			reflector = draw->PShader->Common.Reflector;
			for (Bind& bind : draw->PSBinds)
			{
				ResolveBind(bind, reflector, draw->PShader->Common.ShaderPath);
			}
			PrepareConstants(device, reflector, draw->PSCBs, draw->PSConstants,
				draw->PShader->Common.ShaderPath);
		}

		size_t blendCount = draw->BlendStates.size();
		if (blendCount > 0)
		{
			InitAssert(blendCount <= 8, "Too blend states on draw, max is 8.");
			D3D11_BLEND_DESC desc = {};
			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = blendCount > 1;
			for (int i = 0 ; i < blendCount ; ++i)
			{
				BlendState* blend = draw->BlendStates[i];
				desc.RenderTarget[i].BlendEnable = blend->Enable;
				desc.RenderTarget[i].SrcBlend = RlfToD3d(blend->Src);
				desc.RenderTarget[i].DestBlend = RlfToD3d(blend->Dest);
				desc.RenderTarget[i].BlendOp = RlfToD3d(blend->Op);
				desc.RenderTarget[i].SrcBlendAlpha = RlfToD3d(blend->SrcAlpha);
				desc.RenderTarget[i].DestBlendAlpha = RlfToD3d(blend->DestAlpha);
				desc.RenderTarget[i].BlendOpAlpha = RlfToD3d(blend->OpAlpha);
				desc.RenderTarget[i].RenderTargetWriteMask = blend->RenderTargetWriteMask;
			}
			HRESULT hr = device->CreateBlendState(&desc, &draw->BlendGfxState);
			CheckHresult(hr, "BlendState");
		}
	}

	ast::EvaluationContext evCtx;
	evCtx.DisplaySize = displaySize;
	evCtx.Time = 0;

	// Size expressions may depend on constants so we need to evaluate them first
	EvaluateConstants(evCtx, rd->Constants);

	for (Buffer* buf : rd->Buffers)
	{
		// Obj initialized buffers don't have expressions.
		if (buf->ElementSizeExpr || buf->ElementCountExpr)
		{
			ast::Result res;
			EvaluateExpression(evCtx, buf->ElementSizeExpr, res, UintType, "Buffer::ElementSize");
			buf->ElementSize = res.Value.UintVal;
			EvaluateExpression(evCtx, buf->ElementCountExpr, res, UintType, "Buffer::ElementCount");
			buf->ElementCount = res.Value.UintVal;
		}

		CreateBuffer(device, buf);
	}

	for (Texture* tex : rd->Textures)
	{
		if (tex->DDSPath)
		{
			std::string ddsPath = dirPath + tex->DDSPath;
			HANDLE dds = fileio::OpenFileOptional(ddsPath.c_str(), GENERIC_READ);
			InitAssert(dds != INVALID_HANDLE_VALUE, "Couldn't find DDS file: %s", 
				ddsPath.c_str());

			u32 ddsSize = fileio::GetFileSize(dds);
			char* ddsBuffer = (char*)malloc(ddsSize);
			Assert(ddsBuffer != nullptr, "failed to alloc");

			fileio::ReadFile(dds, ddsBuffer, ddsSize);

			CloseHandle(dds);

			DirectX::TexMetadata meta;
			DirectX::ScratchImage scratch;
			DirectX::LoadFromDDSMemory(ddsBuffer, ddsSize, DirectX::DDS_FLAGS_NONE, 
				&meta, scratch);
			ID3D11Resource* res;
			free(ddsBuffer);
			HRESULT hr = DirectX::CreateTextureEx(device, scratch.GetImages(),
				scratch.GetImageCount(), meta, D3D11_USAGE_IMMUTABLE, 
				D3D11_BIND_SHADER_RESOURCE, 0, 0, false, &res);
			Assert(hr == S_OK, "Failed to create texture from DDS, hr=%x", hr);
			hr = res->QueryInterface(IID_ID3D11Texture2D, (void**)&tex->GfxState);
			SafeRelease(res);
			Assert(hr == S_OK, "Failed to query texture object, hr=%x", hr);
		}
		else
		{
			ast::Result res;
			EvaluateExpression(evCtx, tex->SizeExpr, res, Uint2Type, "Texture::Size");
			tex->Size = res.Value.Uint2Val;

			CreateTexture(device, tex);
		}
	}

	for (View* v : rd->Views)
	{
		CreateView(device, v);
	}

	for (Sampler* s : rd->Samplers)
	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = RlfToD3d(s->Filter);
		desc.AddressU = RlfToD3d(s->AddressMode.U);
		desc.AddressV = RlfToD3d(s->AddressMode.V);
		desc.AddressW = RlfToD3d(s->AddressMode.W);
		desc.MipLODBias = s->MipLODBias;
		desc.MaxAnisotropy = s->MaxAnisotropy;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.BorderColor[0] = s->BorderColor.x;
		desc.BorderColor[1] = s->BorderColor.y;
		desc.BorderColor[2] = s->BorderColor.z;
		desc.BorderColor[3] = s->BorderColor.w;
		desc.MinLOD = s->MinLOD;
		desc.MaxLOD = s->MaxLOD;

		HRESULT hr = device->CreateSamplerState(&desc, &s->GfxState);
		Assert(hr == S_OK, "failed to create sampler, hr=%x", hr);
	}

	for (RasterizerState* rs : rd->RasterizerStates)
	{
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = rs->Fill ? D3D11_FILL_SOLID : D3D11_FILL_WIREFRAME;
		desc.CullMode = RlfToD3d(rs->CullMode);
		desc.FrontCounterClockwise = rs->FrontCCW;
		desc.DepthBias = rs->DepthBias;
		desc.SlopeScaledDepthBias = rs->SlopeScaledDepthBias;
		desc.DepthBiasClamp = rs->DepthBiasClamp;
		desc.DepthClipEnable = rs->DepthClipEnable;
		desc.ScissorEnable = rs->ScissorEnable;
		desc.MultisampleEnable = rs->MultisampleEnable;
		desc.AntialiasedLineEnable = rs->AntialiasedLineEnable;
		device->CreateRasterizerState(&desc, &rs->GfxState);
	}

	for (DepthStencilState* dss : rd->DepthStencilStates)
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.DepthEnable = dss->DepthEnable;
		desc.DepthWriteMask = dss->DepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : 
			D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = RlfToD3d(dss->DepthFunc);
		desc.StencilEnable = dss->StencilEnable;
		desc.StencilReadMask = dss->StencilReadMask;
		desc.StencilWriteMask = dss->StencilWriteMask;
		desc.FrontFace = RlfToD3d(dss->FrontFace);
		desc.BackFace = RlfToD3d(dss->BackFace);
		device->CreateDepthStencilState(&desc, &dss->GfxState);
	}
}

void ReleaseD3D(
	gfx::Context*,
	RenderDescription* rd)
{
	SafeRelease(DefaultRasterizerState);

	for (ComputeShader* cs : rd->CShaders)
	{
		SafeRelease(cs->GfxState);
		SafeRelease(cs->Common.Reflector);
	}
	for (VertexShader* vs : rd->VShaders)
	{
		SafeRelease(vs->GfxState);
		SafeRelease(vs->Common.Reflector);
		SafeRelease(vs->LayoutGfxState);
	}
	for (PixelShader* ps : rd->PShaders)
	{
		SafeRelease(ps->GfxState);
		SafeRelease(ps->Common.Reflector);
	}

	for (Buffer* buf : rd->Buffers)
	{
		SafeRelease(buf->GfxState);
	}

	for (Texture* tex : rd->Textures)
	{
		SafeRelease(tex->GfxState);
	}

	for (View* v : rd->Views)
	{
		ReleaseView(v);
	}

	for (Sampler* s : rd->Samplers)
	{
		SafeRelease(s->GfxState);
	}

	for (RasterizerState* rs : rd->RasterizerStates)
	{
		SafeRelease(rs->GfxState);
	}
	for (DepthStencilState* dss : rd->DepthStencilStates)
	{
		SafeRelease(dss->GfxState);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		for (ConstantBuffer& cb : dc->CBs)
		{
			SafeRelease(cb.GfxState);
			free(cb.BackingMemory);
		}
	}

	for (Draw* d : rd->Draws)
	{
		SafeRelease(d->BlendGfxState);
		for (ConstantBuffer& cb : d->VSCBs)
		{
			SafeRelease(cb.GfxState);
			free(cb.BackingMemory);
		}
		for (ConstantBuffer& cb : d->PSCBs)
		{
			SafeRelease(cb.GfxState);
			free(cb.BackingMemory);
		}
	}
}

void HandleTextureParametersChanged(
	RenderDescription* rd,
	ExecuteContext* ec,
	ErrorState* errorState)
{
	ID3D11Device* device = ec->GfxCtx->Device;
	errorState->Success = true;
	errorState->Warning = false;
	try {
		// Size expressions may depend on constants so we need to evaluate them first
		EvaluateConstants(ec->EvCtx, rd->Constants);

		for (Texture* tex : rd->Textures)
		{
			// DDS textures are always sized based on the file. 
			if (tex->DDSPath)
				continue;
			if ((tex->SizeExpr->Dep.VariesByFlags & ec->EvCtx.ChangedThisFrameFlags) == 0)
				continue;
			
			ast::Result res;
			EvaluateExpression(ec->EvCtx, tex->SizeExpr, res, Uint2Type, "Texture::Size");
			uint2 newSize = res.Value.Uint2Val;

			if (tex->Size != newSize)
			{
				tex->Size = newSize;

				SafeRelease(tex->GfxState);
				
				CreateTexture(device, tex);

				// Find any views that use this texture and recreate them. 
				for (View* view : tex->Views)
				{
					if (view->ResourceType == ResourceType::Texture && 
						view->Texture == tex)
					{
						ReleaseView(view);
						CreateView(device, view);
					}
				}
			}
		}

		for (Buffer* buf : rd->Buffers)
		{
			// Obj initialized buffers don't have expressions.
			if (!buf->ElementSizeExpr && !buf->ElementCountExpr)
				continue;

			if ((buf->ElementSizeExpr->Dep.VariesByFlags & ec->EvCtx.ChangedThisFrameFlags) == 0 &&
				(buf->ElementCountExpr->Dep.VariesByFlags & ec->EvCtx.ChangedThisFrameFlags) == 0)
				continue;
			
			ast::Result res;
			EvaluateExpression(ec->EvCtx, buf->ElementSizeExpr, res, UintType, "Buffer::ElementSize");
			u32 newSize = res.Value.UintVal;
			EvaluateExpression(ec->EvCtx, buf->ElementCountExpr, res, UintType, "Buffer::ElementCount");
			u32 newCount = res.Value.UintVal;

			if (buf->ElementSize != newSize || buf ->ElementCount != newCount)
			{
				buf->ElementSize = newSize;
				buf->ElementCount = newCount;

				SafeRelease(buf->GfxState);
				
				CreateBuffer(device, buf);

				// Find any views that use this buffer and recreate them. 
				for (View* view : buf->Views)
				{
					if (view->ResourceType == ResourceType::Buffer && 
						view->Buffer == buf)
					{
						ReleaseView(view);
						CreateView(device, view);
					}
				}
			}
		}
	}
	catch (ErrorInfo ie)
	{
		errorState->Success = false;
		errorState->Info = ie;
	}
}



// -----------------------------------------------------------------------------
// ------------------------------ EXECUTION ------------------------------------
// -----------------------------------------------------------------------------
void ExecuteError(const char* str, ...)
{
	char buf[512];
	va_list ptr;
	va_start(ptr,str);
	vsprintf_s(buf,512,str,ptr);
	va_end(ptr);

	ErrorInfo ee;
	ee.Location = nullptr;
	ee.Message = buf;
	throw ee;
}

#define ExecuteAssert(expression, message, ...) \
do {											\
	if (!(expression)) {						\
		ExecuteError(message, ##__VA_ARGS__);	\
	}											\
} while (0);									\

void ExecuteSetConstants(ExecuteContext* ec, std::vector<SetConstant>& sets, 
	std::vector<ConstantBuffer>& buffers)
{
	ID3D11DeviceContext* ctx = ec->GfxCtx->DeviceContext;
	for (const SetConstant& set : sets)
	{
		ast::Result res;
		EvaluateExpression(ec->EvCtx, set.Value, res, set.Type, set.VariableName);
		u32 typeSize = res.Type.Dim * 4;
		Assert(set.Size == typeSize, 
			"SetConstant %s does not match size, expected=%u got=%u",
			set.VariableName, set.Size, typeSize);
		if (res.Type.Fmt == VariableFormat::Bool)
			for (u32 i = 0 ; i < res.Type.Dim ; ++i)
				*(((u32*)(set.CB->BackingMemory+set.Offset)) + i) = 
					res.Value.Bool4Val.m[i] ? 1 : 0;
		else
			memcpy(set.CB->BackingMemory+set.Offset, &res.Value, typeSize);
	}
	for (const ConstantBuffer& buf : buffers)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		HRESULT hr = ctx->Map(buf.GfxState, 0, D3D11_MAP_WRITE_DISCARD, 
			0, &mapped_resource);
		Assert(hr == S_OK, "failed to map CB hr=%x", hr);
		memcpy(mapped_resource.pData, buf.BackingMemory, buf.Size);
		ctx->Unmap(buf.GfxState, 0);
	}
}

void ExecuteDispatch(
	Dispatch* dc,
	ExecuteContext* ec)
{
	ID3D11DeviceContext* ctx = ec->GfxCtx->DeviceContext;
	UINT initialCount = (UINT)-1;
	ctx->CSSetShader(dc->Shader->GfxState, nullptr, 0);
	ExecuteSetConstants(ec, dc->Constants, dc->CBs);
	for (const ConstantBuffer& buf : dc->CBs)
	{
		ctx->CSSetConstantBuffers(buf.Slot, 1, &buf.GfxState);
	}
	for (Bind& bind : dc->Binds)
	{
		switch (bind.Type)
		{
		case BindType::SystemValue:
			Assert(bind.IsOutput, "Invalid");
			if (bind.SystemBind == SystemValue::BackBuffer)
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &ec->Res.MainRtUav, 
					&initialCount);
			else
				Assert(false, "unhandled");
			break;
		case BindType::View:
			if (bind.IsOutput)
			{
				Assert(bind.ViewBind->Type == ViewType::UAV, "Invalid");
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, 
					&bind.ViewBind->UAVGfxState, &initialCount);
			}
			else
			{
				Assert(bind.ViewBind->Type == ViewType::SRV, "Invalid");
				ctx->CSSetShaderResources(bind.BindIndex, 1, 
					&bind.ViewBind->SRVGfxState);
			}
			break;
		case BindType::Sampler:
			ctx->CSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->GfxState);
			break;
		default:
			Assert(false, "invalid type %d", bind.Type);
		}
	}
	if (dc->IndirectArgs)
	{
		ctx->DispatchIndirect(dc->IndirectArgs->GfxState, dc->IndirectArgsOffset);
	}
	else
	{
		uint3 groups = {};
		if (dc->ThreadPerPixel)
		{
			Assert(ec->EvCtx.DisplaySize.x != 0 && ec->EvCtx.DisplaySize.y != 0,
				"Invalid display size for execution");
			uint3 tgs = dc->Shader->ThreadGroupSize;
			groups.x = (u32)((ec->EvCtx.DisplaySize.x - 1) / tgs.x) + 1;
			groups.y = (u32)((ec->EvCtx.DisplaySize.y - 1) / tgs.y) + 1;
			groups.z = 1;
		}
		else if (dc->Groups)
		{
			ast::Result res;
			EvaluateExpression(ec->EvCtx, dc->Groups, res, Uint3Type, "Dispatch::Groups");
			groups = res.Value.Uint3Val;
		}

		ctx->Dispatch(groups.x, groups.y, groups.z);
	}
}

void ExecuteDraw(
	Draw* draw,
	ExecuteContext* ec)
{
	ID3D11DeviceContext* ctx = ec->GfxCtx->DeviceContext;
	ctx->VSSetShader(draw->VShader->GfxState, nullptr, 0);
	ctx->IASetInputLayout(draw->VShader->LayoutGfxState);
	ctx->PSSetShader(draw->PShader ? draw->PShader->GfxState : nullptr, nullptr, 0);
	ExecuteSetConstants(ec, draw->VSConstants, draw->VSCBs);
	ExecuteSetConstants(ec, draw->PSConstants, draw->PSCBs);
	for (const ConstantBuffer& buf : draw->VSCBs)
	{
		ctx->VSSetConstantBuffers(buf.Slot, 1, &buf.GfxState);
	}
	for (const ConstantBuffer& buf : draw->PSCBs)
	{
		ctx->PSSetConstantBuffers(buf.Slot, 1, &buf.GfxState);
	}
	for (Bind& bind : draw->VSBinds)
	{
		switch (bind.Type)
		{
		case BindType::View:
		{
			ExecuteAssert(bind.ViewBind->Type == ViewType::SRV, 
				"UAVs are not supported in vertex shaders.");
			ctx->VSSetShaderResources(bind.BindIndex, 1, &bind.ViewBind->SRVGfxState);
			break;
		}
		case BindType::Sampler:
			ctx->VSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->GfxState);
			break;
		case BindType::SystemValue:
		default:
			Assert(false, "unhandled type %d", bind.Type);
		}
	}
	ID3D11UnorderedAccessView *uavs[D3D11_PS_CS_UAV_REGISTER_COUNT] = {};
	u32 uav_min = 0xffffffff;
	u32 uav_max = 0;
	for (Bind& bind : draw->PSBinds)
	{
		switch (bind.Type)
		{
		case BindType::View:
		{
			if (bind.IsOutput)
			{
				Assert(bind.ViewBind->Type == ViewType::UAV, "Invalid");
				uav_min = min(uav_min, bind.BindIndex);
				uav_max = max(uav_max, bind.BindIndex);
				uavs[bind.BindIndex] = bind.ViewBind->UAVGfxState;
			}
			else
			{
				Assert(bind.ViewBind->Type == ViewType::SRV, "Invalid");
				ctx->PSSetShaderResources(bind.BindIndex, 1, &bind.ViewBind->SRVGfxState);
			}
			break;
		}
		case BindType::Sampler:
			ctx->PSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->GfxState);
			break;
		case BindType::SystemValue:
		default:
			Assert(false, "unhandled type %d", bind.Type);
		}
	}
	ID3D11RenderTargetView* rtViews[8] = {};
	D3D11_VIEWPORT vp[8] = {};
	u32 rtCount = 0;
	for (TextureTarget target : draw->RenderTargets)
	{
		if (target.IsSystem)
		{
			if (target.System == SystemValue::BackBuffer)
			{
				rtViews[rtCount] = ec->Res.MainRtv;
				vp[rtCount].Width = (float)ec->EvCtx.DisplaySize.x;
				vp[rtCount].Height = (float)ec->EvCtx.DisplaySize.y;
			}
			else 
				Unimplemented();
		}
		else
		{
			Assert(target.View->Type == ViewType::RTV, "Invalid");
			Assert(target.View->ResourceType == ResourceType::Texture, "Invalid");
			rtViews[rtCount] = target.View->RTVGfxState;
			vp[rtCount].Width = (float)target.View->Texture->Size.x;
			vp[rtCount].Height = (float)target.View->Texture->Size.y;
		}
		vp[rtCount].MinDepth = 0.0f;
		vp[rtCount].MaxDepth = 1.0f;
		vp[rtCount].TopLeftX = vp[rtCount].TopLeftY = 0;
		++rtCount;
	}
	ID3D11DepthStencilView* dsView = nullptr;
	for (TextureTarget target : draw->DepthStencil)
	{
		if (target.IsSystem)
		{
			if (target.System == SystemValue::DefaultDepth)
			{
				dsView = ec->Res.DefaultDepthView;
				vp[0].Width = (float)ec->EvCtx.DisplaySize.x;
				vp[0].Height = (float)ec->EvCtx.DisplaySize.y;
			}
			else
				Unimplemented();
		}
		else
		{
			Assert(target.View->Type == ViewType::DSV, "Invalid");
			Assert(target.View->ResourceType == ResourceType::Texture, "Invalid");
			dsView = target.View->DSVGfxState;
			vp[0].Width = (float)target.View->Texture->Size.x;
			vp[0].Height = (float)target.View->Texture->Size.y;
		}
		vp[0].MinDepth = 0.0f;
		vp[0].MaxDepth = 1.0f;
	}
	for (int i = 0 ; i < draw->Viewports.size() ; ++i)
	{
		Viewport* v = draw->Viewports[i];
		ast::Result res;
		if (v->TopLeft) {
			EvaluateExpression(ec->EvCtx, v->TopLeft, res, Float2Type, "Viewport::TopLeft");
			vp[i].TopLeftX = res.Value.Float2Val.x;
			vp[i].TopLeftY = res.Value.Float2Val.y;
		}
		if (v->Size) {
			EvaluateExpression(ec->EvCtx, v->Size, res, Float2Type, "Viewport::Size");
			vp[i].Width = res.Value.Float2Val.x;
			vp[i].Height = res.Value.Float2Val.y;
		}
		if (v->DepthRange) {
			EvaluateExpression(ec->EvCtx, v->DepthRange, res, Float2Type, "Viewport::DepthRange");
			vp[i].MinDepth = res.Value.Float2Val.x;
			vp[i].MaxDepth = res.Value.Float2Val.y;
		}
	}
	ctx->OMSetRenderTargetsAndUnorderedAccessViews(rtCount, rtViews, dsView, uav_min, 
		uav_max-uav_min+1, uav_min != 0xffffffff ? uavs+uav_min : nullptr, nullptr);
	ctx->RSSetViewports(8, vp);
	ctx->IASetPrimitiveTopology(RlfToD3d(draw->Topology));
	ctx->RSSetState(draw->RState ? draw->RState->GfxState : DefaultRasterizerState);
	ctx->OMSetDepthStencilState(draw->DSState ? draw->DSState->GfxState : nullptr,
		draw->StencilRef);
	ctx->OMSetBlendState(draw->BlendGfxState, nullptr, 0xffffffff);
	if (!draw->VertexBuffers.empty())
	{
		ID3D11Buffer* bufs[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
		u32 elementSizes[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
		u32 offsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
		for (u32 i = 0 ; i < draw->VertexBuffers.size() ; ++i)
		{
			Buffer* vb = draw->VertexBuffers[i];
			bufs[i] = vb->GfxState;
			elementSizes[i] = vb->ElementSize;
			offsets[i] = 0;
		}
		ctx->IASetVertexBuffers(0, (u32)draw->VertexBuffers.size(), bufs, elementSizes, 
			offsets);
	}
	Buffer* ib = draw->IndexBuffer;
	if (ib)
		ctx->IASetIndexBuffer(ib->GfxState, ib->ElementSize == 2 ? 
			DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);

	if (draw->InstancedIndirectArgs)
		ctx->DrawInstancedIndirect(draw->InstancedIndirectArgs->GfxState,
			draw->IndirectArgsOffset);
	else if (draw->IndexedInstancedIndirectArgs)
		ctx->DrawIndexedInstancedIndirect(draw->IndexedInstancedIndirectArgs->GfxState,
			draw->IndirectArgsOffset);
	else if (ib && draw->InstanceCount > 0)
		ctx->DrawIndexedInstanced(ib->ElementCount, draw->InstanceCount, 0, 0, 0);
	else if (ib)
		ctx->DrawIndexed(ib->ElementCount, 0, 0);
	else if (draw->InstanceCount > 0)
		ctx->DrawInstanced(draw->VertexCount, draw->InstanceCount, 0, 0);
	else
		ctx->Draw(draw->VertexCount, 0);
}

void _Execute(
	ExecuteContext* ec,
	RenderDescription* rd)
{
	ID3D11DeviceContext* ctx = ec->GfxCtx->DeviceContext;

	EvaluateConstants(ec->EvCtx, rd->Constants);

	// Clear state so we aren't polluted by previous program drawing or previous 
	//	execution. 
	ctx->ClearState();

	for (Pass pass : rd->Passes)
	{
		if (pass.Type == PassType::Dispatch)
		{
			ExecuteDispatch(pass.Dispatch, ec);
		}
		else if (pass.Type == PassType::Draw)
		{
			ExecuteDraw(pass.Draw, ec);
		}
		else if (pass.Type == PassType::ClearColor)
		{
			float4& color = pass.ClearColor->Color;
			const float clear_color[4] =
			{
				color.x, color.y, color.z, color.w
			};
			ctx->ClearRenderTargetView(pass.ClearColor->Target->RTVGfxState, 
				clear_color);
		}
		else if (pass.Type == PassType::ClearDepth)
		{
			ctx->ClearDepthStencilView(pass.ClearDepth->Target->DSVGfxState, 
				D3D11_CLEAR_DEPTH, pass.ClearDepth->Depth, 0);
		}
		else if (pass.Type == PassType::ClearStencil)
		{
			ctx->ClearDepthStencilView(pass.ClearStencil->Target->DSVGfxState, 
				D3D11_CLEAR_STENCIL, 0.f, pass.ClearStencil->Stencil);
		}
		else if (pass.Type == PassType::Resolve)
		{
			ctx->ResolveSubresource(pass.Resolve->Dst->GfxState, 0, 
				pass.Resolve->Src->GfxState, 0, 
				D3DTextureFormat[(u32)pass.Resolve->Dst->Format]);
		}
		else
		{
			Unimplemented();
		}

		// Clear state after execution so we don't pollute the rest of program drawing. 
		ctx->ClearState();
	}

}

#undef ExecuteAssert
#undef SafeRelease

} // namespace rlf
