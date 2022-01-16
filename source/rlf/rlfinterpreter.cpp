
namespace rlf
{

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

ID3DBlob* CommonCompileShader(const char* path, const char* profile, 
	const char * entry, ErrorState* errorState)
{
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
		D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, profile, D3DCOMPILE_DEBUG, 
		0, &shaderBlob, &errorBlob);

	free(shaderBuffer);

	bool success = (hr == S_OK);

	if (!success)
	{
		Assert(errorBlob != nullptr, "No error info given for shader compile fail.");
		char* errorText = (char*)errorBlob->GetBufferPointer();
		std::string textCopy = errorText;

		Assert(shaderBlob == nullptr, "leak");
		SafeRelease(errorBlob);

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
	return shaderBlob;
}

void CreateInputLayout(ID3D11Device* device, VertexShader* shader, ID3DBlob* blob)
{
	ID3D11ShaderReflection* reflector = shader->Reflector;

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	reflector->GetDesc( &shaderDesc );

	// Read input layout description from shader info
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
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

	if (inputLayoutDesc.size())
	{
		// Try to create Input Layout
		HRESULT hr = device->CreateInputLayout(
			&inputLayoutDesc[0], (u32)inputLayoutDesc.size(), blob->GetBufferPointer(), 
			blob->GetBufferSize(), &shader->InputLayout);
		Assert(hr == S_OK, "failed to create input layout, hr=%x", hr);
	}
}

void CreateTexture(ID3D11Device* device, Texture* tex)
{
	Assert(tex->TextureObject == nullptr, "Leaking object");

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = tex->Size.x;
	desc.Height = tex->Size.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = D3DTextureFormat[(u32)tex->Format];
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = RlfToD3d(tex->Flags);
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	HRESULT hr = device->CreateTexture2D(&desc, nullptr, &tex->TextureObject);
	CheckHresult(hr, "Texture");
}

void CreateView(ID3D11Device* device, View* v)
{
	ID3D11Resource* res = nullptr;
	if (v->ResourceType == ResourceType::Buffer)
		res = v->Buffer->BufferObject;
	else if (v->ResourceType == ResourceType::Texture)
		res = v->Texture->TextureObject;
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
			vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			vd.Texture2D.MostDetailedMip = 0;
			vd.Texture2D.MipLevels = (u32)-1;
		}
		else 
			Unimplemented();
		HRESULT hr = device->CreateShaderResourceView(res, &vd, &v->SRVObject);
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
		HRESULT hr = device->CreateUnorderedAccessView(res, &vd, &v->UAVObject);
		CheckHresult(hr, "UAV");
	}
	else if (v->Type == ViewType::RTV)
	{
		D3D11_RENDER_TARGET_VIEW_DESC vd;
		vd.Format = fmt;
		Assert(v->ResourceType == ResourceType::Texture, "Invalid");
		vd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		vd.Texture2D.MipSlice = 0;
		HRESULT hr = device->CreateRenderTargetView(res, &vd, &v->RTVObject);
		CheckHresult(hr, "RTV");
	}
	else if (v->Type == ViewType::DSV)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC vd;
		vd.Format = fmt;
		Assert(v->ResourceType == ResourceType::Texture, "Invalid");
		vd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		vd.Texture2D.MipSlice = 0;
		vd.Flags = 0;
		HRESULT hr = device->CreateDepthStencilView(res, &vd, &v->DSVObject);
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
		SafeRelease(v->SRVObject);
		break;
	case ViewType::UAV:
		SafeRelease(v->UAVObject);
		break;
	case ViewType::RTV:
		SafeRelease(v->RTVObject);
		break;
	case ViewType::DSV:
		SafeRelease(v->DSVObject);
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
	ID3D11ShaderReflection* reflector, std::vector<ConstantBuffer>& buffers, 
	std::vector<SetConstant>& sets, const char* path)
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
		HRESULT hr = g_pd3dDevice->CreateBuffer(&desc, &srd, &cb.BufferObject);
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
		Assert(cvar, "Couldn't find constant %s in shader %s", set.VariableName, path);
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
	ID3D11Device* device,
	RenderDescription* rd,
	uint2 displaySize,
	const char* workingDirectory,
	ErrorState* errorState)
{
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
		std::string shaderPath = dirPath + cs->ShaderPath;
		ID3DBlob* shaderBlob = CommonCompileShader(shaderPath.c_str(), "cs_5_0", 
			cs->EntryPoint, errorState);

		HRESULT hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &cs->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &cs->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		cs->Reflector->GetThreadGroupSize(&cs->ThreadGroupSize.x, &cs->ThreadGroupSize.y,
			&cs->ThreadGroupSize.z);

		SafeRelease(shaderBlob);
	}

	for (VertexShader* vs : rd->VShaders)
	{
		std::string shaderPath = dirPath + vs->ShaderPath;
		ID3DBlob* shaderBlob = CommonCompileShader(shaderPath.c_str(), "vs_5_0", 
			vs->EntryPoint, errorState);

		HRESULT hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &vs->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &vs->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		CreateInputLayout(device, vs, shaderBlob);

		SafeRelease(shaderBlob);
	}

	for (PixelShader* ps : rd->PShaders)
	{
		std::string shaderPath = dirPath + ps->ShaderPath;
		ID3DBlob* shaderBlob = CommonCompileShader(shaderPath.c_str(), "ps_5_0", 
			ps->EntryPoint, errorState);

		HRESULT hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), 
			shaderBlob->GetBufferSize(), NULL, &ps->ShaderObject);
		Assert(hr == S_OK, "Failed to create shader, hr=%x", hr);

		hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D11ShaderReflection, (void**) &ps->Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		SafeRelease(shaderBlob);
	}


	for (Dispatch* dc : rd->Dispatches)
	{
		ID3D11ShaderReflection* reflector = dc->Shader->Reflector;
		for (Bind& bind : dc->Binds)
		{
			ResolveBind(bind, reflector, dc->Shader->ShaderPath);
		}
		PrepareConstants(reflector, dc->CBs, dc->Constants, dc->Shader->ShaderPath);
	}

	for (Draw* draw : rd->Draws)
	{
		InitAssert(draw->VShader, "Null vertex shader on draw not permitted.");
		ID3D11ShaderReflection* reflector = draw->VShader->Reflector;
		for (Bind& bind : draw->VSBinds)
		{
			ResolveBind(bind, reflector, draw->VShader->ShaderPath);
		}
		PrepareConstants(reflector, draw->VSCBs, draw->VSConstants,
			draw->VShader->ShaderPath);
		if (draw->PShader)
		{
			reflector = draw->PShader->Reflector;
			for (Bind& bind : draw->PSBinds)
			{
				ResolveBind(bind, reflector, draw->PShader->ShaderPath);
			}
			PrepareConstants(reflector, draw->PSCBs, draw->PSConstants,
				draw->PShader->ShaderPath);
		}
	}

	for (Buffer* buf : rd->Buffers)
	{
		u32 bufSize = buf->ElementSize * buf->ElementCount;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = buf->InitData;

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = bufSize;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = RlfToD3d_Bind(buf->Flags);
		desc.CPUAccessFlags = 0;
		desc.StructureByteStride = buf->ElementSize;
		desc.MiscFlags = RlfToD3d_Misc(buf->Flags); 

		HRESULT hr = device->CreateBuffer(&desc, initData.pSysMem ? &initData : nullptr,
			&buf->BufferObject);
		CheckHresult(hr, "Buffer");
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
			HRESULT hr = DirectX::CreateTextureEx(device, scratch.GetImages(),
				scratch.GetImageCount(), meta, D3D11_USAGE_IMMUTABLE, 
				D3D11_BIND_SHADER_RESOURCE, 0, 0, false, &res);
			Assert(hr == S_OK, "Failed to create texture from DDS, hr=%x", hr);
			hr = res->QueryInterface(IID_ID3D11Texture2D, (void**)&tex->TextureObject);
			Assert(hr == S_OK, "Failed to query texture object, hr=%x", hr);
		}
		else
		{
			ast::EvaluationContext evCtx;
			evCtx.DisplaySize = displaySize;
			evCtx.Time = 0;
			ast::Result res;
			ErrorState es;
			ast::Evaluate(evCtx, tex->SizeExpr, res, es);
			if (!es.Success)
			{
				ErrorInfo ie;
				ie.Location = es.Info.Location;
				ie.Message = "AST evaluation error: " + es.Info.Message;
				throw ie;
			}
			InitAssert( res.Type.Dim == 2, 
				"Size expression expected to evaluate to 2 components but got: %d",
				res.Type.Dim);
			Convert(res, VariableFormat::Uint);
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

		HRESULT hr = device->CreateSamplerState(&desc, &s->SamplerObject);
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
		device->CreateRasterizerState(&desc, &rs->RSObject);
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
		device->CreateDepthStencilState(&desc, &dss->DSSObject);
	}
}

void InitD3D(
	ID3D11Device* device,
	ID3D11InfoQueue* infoQueue,
	RenderDescription* rd,
	uint2 displaySize,
	const char* workingDirectory,
	ErrorState* errorState)
{
	gInfoQueue = infoQueue;
	errorState->Success = true;
	errorState->Warning = false;
	try {
		InitMain(device, rd, displaySize, workingDirectory, errorState);
	}
	catch (ErrorInfo ie)
	{
		errorState->Success = false;
		errorState->Info = ie;
	}
}

void ReleaseD3D(
	RenderDescription* rd)
{
	for (ComputeShader* cs : rd->CShaders)
	{
		SafeRelease(cs->ShaderObject);
		SafeRelease(cs->Reflector);
	}
	for (VertexShader* vs : rd->VShaders)
	{
		SafeRelease(vs->ShaderObject);
		SafeRelease(vs->Reflector);
		SafeRelease(vs->InputLayout);
	}
	for (PixelShader* ps : rd->PShaders)
	{
		SafeRelease(ps->ShaderObject);
		SafeRelease(ps->Reflector);
	}

	for (Buffer* buf : rd->Buffers)
	{
		SafeRelease(buf->BufferObject);
	}

	for (Texture* tex : rd->Textures)
	{
		SafeRelease(tex->TextureObject);
	}

	for (View* v : rd->Views)
	{
		ReleaseView(v);
	}

	for (Sampler* s : rd->Samplers)
	{
		SafeRelease(s->SamplerObject);
	}

	for (RasterizerState* rs : rd->RasterizerStates)
	{
		SafeRelease(rs->RSObject);
	}
	for (DepthStencilState* dss : rd->DepthStencilStates)
	{
		SafeRelease(dss->DSSObject);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		for (ConstantBuffer& cb : dc->CBs)
		{
			SafeRelease(cb.BufferObject);
			free(cb.BackingMemory);
		}
	}
}

void HandleTextureParametersChanged(
	ID3D11Device* device,
	RenderDescription* rd,
	uint2 displaySize,
	u32 changedFlags,
	ErrorState* errorState)
{
	errorState->Success = true;
	errorState->Warning = false;
	try {
		for (Texture* tex : rd->Textures)
		{
			// DDS textures are always sized based on the file. 
			if (tex->DDSPath)
				continue;
			if ((tex->SizeExpr->Dep.VariesByFlags & changedFlags) == 0)
				continue;
			ast::EvaluationContext evCtx;
			evCtx.DisplaySize = displaySize;
			evCtx.Time = 0;
			ast::Result res;
			ErrorState es;
			ast::Evaluate(evCtx, tex->SizeExpr, res, es);
			if (!es.Success)
			{
				ErrorInfo ie;
				ie.Location = es.Info.Location;
				ie.Message = "AST evaluation error: " + es.Info.Message;
				throw ie;
			}
			InitAssert( res.Type.Dim == 2, 
				"Size expression expected to evaluate to 2 components but got: %d",
				res.Type.Dim);
			Convert(res, VariableFormat::Uint);
			uint2 newSize = res.Value.Uint2Val;

			if (tex->Size != newSize)
			{
				tex->Size = newSize;

				SafeRelease(tex->TextureObject);
				
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

void EvaluateConstants(ExecuteContext* ec, std::vector<Constant*>& cnsts)
{
	ast::EvaluationContext evCtx;
	evCtx.DisplaySize = ec->DisplaySize;
	evCtx.Time = ec->Time;
	for (Constant* cnst : cnsts)
	{
		ast::Result res;
		ErrorState es;
		ast::Evaluate(evCtx, cnst->Expr, res, es);
		if (!es.Success)
		{
			ErrorInfo ee;
			ee.Location = es.Info.Location;
			ee.Message = "AST evaluation error: " + es.Info.Message;
			throw ee;
		}
		ExecuteAssert( (cnst->Type.Fmt != VariableFormat::Float4x4 && 
			res.Type.Fmt != VariableFormat::Float4x4) || cnst->Type.Fmt == res.Type.Fmt,
			"Constant %s expected type (%s) is not compatible with actual type (%s)",
			cnst->Name, TypeFmtToString(cnst->Type.Fmt), TypeFmtToString(res.Type.Fmt));
		ExecuteAssert(cnst->Type.Dim == res.Type.Dim,
			"Constant %s size (%u) doe not match actual size (%u)",
			cnst->Name, cnst->Type.Dim, res.Type.Dim);
		Convert(res, cnst->Type.Fmt);
		cnst->Value = res.Value;
	}
}

void ExecuteSetConstants(ExecuteContext* ec, std::vector<SetConstant>& sets, 
	std::vector<ConstantBuffer>& buffers)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;
	ast::EvaluationContext evCtx;
	evCtx.DisplaySize = ec->DisplaySize;
	evCtx.Time = ec->Time;
	for (const SetConstant& set : sets)
	{
		ast::Result res;
		ErrorState es;
		ast::Evaluate(evCtx, set.Value, res, es);
		if (!es.Success)
		{
			ErrorInfo ee;
			ee.Location = es.Info.Location;
			ee.Message = "AST evaluation error: " + es.Info.Message;
			throw ee;
		}
		u32 typeSize = res.Type.Dim * 4;
		ExecuteAssert(set.Size == typeSize, 
			"SetConstant %s does not match size, expected=%u got=%u",
			set.VariableName, set.Size, typeSize);
		if (set.Type != res.Type)
			Convert(res, set.Type.Fmt);
		// ExecuteAssert(set.Type == res.Type, 
		// 	"SetConstant %s does not match type, expected=%s got=%s",
		// 	set.VariableName, TypeToString(set.Type), TypeToString(res.Type));
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
		HRESULT hr = ctx->Map(buf.BufferObject, 0, D3D11_MAP_WRITE_DISCARD, 
			0, &mapped_resource);
		Assert(hr == S_OK, "failed to map CB hr=%x", hr);
		memcpy(mapped_resource.pData, buf.BackingMemory, buf.Size);
		ctx->Unmap(buf.BufferObject, 0);
	}
}

void ExecuteDispatch(
	Dispatch* dc,
	ExecuteContext* ec)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;
	UINT initialCount = (UINT)-1;
	ctx->CSSetShader(dc->Shader->ShaderObject, nullptr, 0);
	ExecuteSetConstants(ec, dc->Constants, dc->CBs);
	for (const ConstantBuffer& buf : dc->CBs)
	{
		ctx->CSSetConstantBuffers(buf.Slot, 1, &buf.BufferObject);
	}
	for (Bind& bind : dc->Binds)
	{
		switch (bind.Type)
		{
		case BindType::SystemValue:
			Assert(bind.IsOutput, "Invalid");
			if (bind.SystemBind == SystemValue::BackBuffer)
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &ec->MainRtUav, 
					&initialCount);
			else
				Assert(false, "unhandled");
			break;
		case BindType::View:
			if (bind.IsOutput)
			{
				Assert(bind.ViewBind->Type == ViewType::UAV, "Invalid");
				ctx->CSSetUnorderedAccessViews(bind.BindIndex, 1, &bind.ViewBind->UAVObject, 
					&initialCount);
			}
			else
			{
				Assert(bind.ViewBind->Type == ViewType::SRV, "Invalid");
				ctx->CSSetShaderResources(bind.BindIndex, 1, &bind.ViewBind->SRVObject);
			}
			break;
		case BindType::Sampler:
			ctx->CSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
			break;
		default:
			Assert(false, "invalid type %d", bind.Type);
		}
	}
	if (dc->Indirect)
	{
		ctx->DispatchIndirect(dc->IndirectArgs->BufferObject, dc->IndirectArgsOffset);
	}
	else
	{
		uint3 groups = dc->Groups;
		if (dc->ThreadPerPixel && ec->DisplaySize.x != 0 && ec->DisplaySize.y != 0)
		{
			uint3 tgs = dc->Shader->ThreadGroupSize;
			groups.x = (u32)((ec->DisplaySize.x - 1) / tgs.x) + 1;
			groups.y = (u32)((ec->DisplaySize.y - 1) / tgs.y) + 1;
			groups.z = 1;
		}

		ctx->Dispatch(groups.x, groups.y, groups.z);
	}
}

void ExecuteDraw(
	Draw* draw,
	ExecuteContext* ec)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;
	ctx->VSSetShader(draw->VShader->ShaderObject, nullptr, 0);
	ctx->IASetInputLayout(draw->VShader->InputLayout);
	ctx->PSSetShader(draw->PShader ? draw->PShader->ShaderObject : nullptr, nullptr, 0);
	ExecuteSetConstants(ec, draw->VSConstants, draw->VSCBs);
	ExecuteSetConstants(ec, draw->PSConstants, draw->PSCBs);
	for (const ConstantBuffer& buf : draw->VSCBs)
	{
		ctx->VSSetConstantBuffers(buf.Slot, 1, &buf.BufferObject);
	}
	for (const ConstantBuffer& buf : draw->PSCBs)
	{
		ctx->PSSetConstantBuffers(buf.Slot, 1, &buf.BufferObject);
	}
	for (Bind& bind : draw->VSBinds)
	{
		switch (bind.Type)
		{
		case BindType::View:
		{
			Assert(bind.ViewBind->Type == ViewType::SRV, "Invalid");
			ctx->VSSetShaderResources(bind.BindIndex, 1, &bind.ViewBind->SRVObject);
			break;
		}
		case BindType::Sampler:
			ctx->VSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
			break;
		case BindType::SystemValue:
		default:
			Assert(false, "unhandled type %d", bind.Type);
		}
	}
	for (Bind& bind : draw->PSBinds)
	{
		switch (bind.Type)
		{
		case BindType::View:
		{
			Assert(bind.ViewBind->Type == ViewType::SRV, "Invalid");
			ctx->PSSetShaderResources(bind.BindIndex, 1, &bind.ViewBind->SRVObject);
			break;
		}
		case BindType::Sampler:
			ctx->PSSetSamplers(bind.BindIndex, 1, &bind.SamplerBind->SamplerObject);
			break;
		case BindType::SystemValue:
		default:
			Assert(false, "unhandled type %d", bind.Type);
		}
	}
	ID3D11RenderTargetView* rtViews[8] = {};
	D3D11_VIEWPORT vp[8] = {};
	u32 rtCount = 0;
	for (TextureTarget target : draw->RenderTarget)
	{
		if (target.IsSystem)
		{
			if (target.System == SystemValue::BackBuffer)
			{
				rtViews[rtCount] = ec->MainRtv;
				vp[rtCount].Width = (float)ec->DisplaySize.x;
				vp[rtCount].Height = (float)ec->DisplaySize.y;
			}
			else 
				Unimplemented();
		}
		else
		{
			Assert(target.View->Type == ViewType::RTV, "Invalid");
			Assert(target.View->ResourceType == ResourceType::Texture, "Invalid");
			rtViews[rtCount] = target.View->RTVObject;
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
				dsView = ec->DefaultDepthView;
				vp[rtCount].Width = (float)ec->DisplaySize.x;
				vp[rtCount].Height = (float)ec->DisplaySize.y;
			}
			else
				Unimplemented();
		}
		else
		{
			Assert(target.View->Type == ViewType::DSV, "Invalid");
			Assert(target.View->ResourceType == ResourceType::Texture, "Invalid");
			dsView = target.View->DSVObject;
			vp[0].Width = (float)target.View->Texture->Size.x;
			vp[0].Height = (float)target.View->Texture->Size.y;
		}
		vp[0].MinDepth = 0.0f;
		vp[0].MaxDepth = 1.0f;
	}
	ctx->OMSetRenderTargets(8, rtViews, dsView);
	ctx->RSSetViewports(8, vp);
	ctx->IASetPrimitiveTopology(RlfToD3d(draw->Topology));
	ctx->RSSetState(draw->RState ? draw->RState->RSObject : DefaultRasterizerState);
	ctx->OMSetDepthStencilState(draw->DSState ? draw->DSState->DSSObject : nullptr,
		draw->StencilRef);
	u32 offset = 0;
	Buffer* vb = draw->VertexBuffer;
	if (vb)
		ctx->IASetVertexBuffers(0, 1, &vb->BufferObject, &vb->ElementSize, 
			&offset);
	if (draw->Type == DrawType::Draw)
	{
		ctx->Draw(draw->VertexCount, 0);
	}
	else if (draw->Type == DrawType::DrawIndexed)
	{
		Buffer* ib = draw->IndexBuffer;
		ExecuteAssert(ib, "DrawIndexed must have an index buffer");
		ctx->IASetIndexBuffer(ib->BufferObject, ib->ElementSize == 2 ? 
			DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
		ctx->DrawIndexed(ib->ElementCount, 0, 0);
	}
	else
	{
		Unimplemented();
	}
}

void _Execute(
	ExecuteContext* ec,
	RenderDescription* rd)
{
	ID3D11DeviceContext* ctx = ec->D3dCtx;

	EvaluateConstants(ec, rd->Constants);

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
			ctx->ClearRenderTargetView(pass.ClearColor->Target->RTVObject, clear_color);
		}
		else if (pass.Type == PassType::ClearDepth)
		{
			ctx->ClearDepthStencilView(pass.ClearDepth->Target->DSVObject, D3D11_CLEAR_DEPTH,
				pass.ClearDepth->Depth, 0);
		}
		else if (pass.Type == PassType::ClearStencil)
		{
			ctx->ClearDepthStencilView(pass.ClearStencil->Target->DSVObject, D3D11_CLEAR_STENCIL,
				0.f, pass.ClearStencil->Stencil);
		}
		else
		{
			Unimplemented();
		}

		// Clear state after execution so we don't pollute the rest of program drawing. 
		ctx->ClearState();
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

#undef ExeucteAssert


} // namespace rlf
