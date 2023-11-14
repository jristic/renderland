
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

static ID3D12InfoQueue*	gInfoQueue = nullptr;

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
		D3D12_MESSAGE* message = (D3D12_MESSAGE*)malloc(messageLength);
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

/* ------TODO-----------

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

*/

D3D12_RESOURCE_FLAGS RlfToD3d(TextureFlag flags)
{
	D3D12_RESOURCE_FLAGS f = D3D12_RESOURCE_FLAG_NONE;
	if (flags & TextureFlag_UAV)
		f |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (flags & TextureFlag_RTV)
		f |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (flags & TextureFlag_DSV)
		f |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	return f;
}

/* ------------ TODO -----------

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

*/

D3D12_CPU_DESCRIPTOR_HANDLE AllocateCbvSrvUavDescriptor(gfx::Context* ctx)
{
	u64 offset = ctx->SrvUavDescNextIndex * ctx->SrvUavDescSize;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = 
		ctx->SrvUavDescHeap->GetCPUDescriptorHandleForHeapStart();
	cpu_descriptor.ptr += offset;

	++ctx->SrvUavDescNextIndex;

	return cpu_descriptor;
}

D3D12_CPU_DESCRIPTOR_HANDLE AllocateRtvDescriptor(gfx::Context* ctx)
{
	u64 offset = ctx->RtvDescNextIndex * ctx->RtvDescSize;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = 
		ctx->RtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	cpu_descriptor.ptr += offset;

	++ctx->RtvDescNextIndex;

	return cpu_descriptor;
}

D3D12_CPU_DESCRIPTOR_HANDLE AllocateDsvDescriptor(gfx::Context* ctx)
{
	u64 offset = ctx->DsvDescNextIndex * ctx->DsvDescSize;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor = 
		ctx->DsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	cpu_descriptor.ptr += offset;

	++ctx->DsvDescNextIndex;

	return cpu_descriptor;
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

void CreateRootSignature(ID3D12Device* device, ComputeShader* cs)
{
	ID3D12ShaderReflection* reflector = cs->Common.Reflector;
	D3D12_SHADER_DESC ShaderDesc = {};
	reflector->GetDesc(&ShaderDesc);

	cs->NumCbvs = 0;
	cs->NumSrvs = 0;
	cs->NumUavs = 0;
	cs->NumSamplers = 0;

	cs->CbvMin = U32_MAX;
	cs->CbvMax = 0;
	cs->SrvMin = U32_MAX;
	cs->SrvMax = 0;
	cs->UavMin = U32_MAX;
	cs->UavMax = 0;
	cs->SamplerMin = U32_MAX;
	cs->SamplerMax = 0;

	for (u32 i = 0 ; i < ShaderDesc.BoundResources ; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC input = {};
		reflector->GetResourceBindingDesc(i, &input);

		InitAssert(input.BindCount == 1, "Multi-bind-point resources are unsupported.");
		InitAssert(input.Space == 0, "Non-zero register spaces are unsupported.");

		switch(input.Type)
		{
		case D3D_SIT_CBUFFER:
			cs->CbvMin = min(cs->CbvMin, input.BindPoint);
			cs->CbvMax = max(cs->CbvMax, input.BindPoint);
			cs->NumCbvs = cs->CbvMax - cs->CbvMin + 1;
			break;
		case D3D_SIT_TBUFFER:
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			cs->SrvMin = min(cs->SrvMin, input.BindPoint);
			cs->SrvMax = max(cs->SrvMax, input.BindPoint);
			cs->NumSrvs = cs->SrvMax - cs->SrvMin + 1;
			break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			cs->UavMin = min(cs->UavMin, input.BindPoint);
			cs->UavMax = max(cs->UavMax, input.BindPoint);
			cs->NumUavs = cs->UavMax - cs->UavMin + 1;
			break;
		case D3D_SIT_SAMPLER:
			cs->SamplerMin = min(cs->SamplerMin, input.BindPoint);
			cs->SamplerMax = max(cs->SamplerMax, input.BindPoint);
			cs->NumSamplers = cs->SamplerMax - cs->SamplerMin + 1;
			break;
		case D3D_SIT_RTACCELERATIONSTRUCTURE:
		case D3D_SIT_UAV_FEEDBACKTEXTURE:
		default:
			Unimplemented();
		}
	}

	D3D12_DESCRIPTOR_RANGE1 ranges[4] = {};
	u32 RangeCount = 0;
	// CBVs
	if (cs->NumCbvs)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[RangeCount].NumDescriptors = cs->CbvMax - cs->CbvMin + 1;
		ranges[RangeCount].BaseShaderRegister = cs->CbvMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 0;
		++RangeCount;
	}
	// SRVs
	if (cs->NumSrvs)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[RangeCount].NumDescriptors = cs->SrvMax - cs->SrvMin + 1;
		ranges[RangeCount].BaseShaderRegister = cs->SrvMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		++RangeCount;
	}
	// UAVs
	if (cs->NumUavs)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[RangeCount].NumDescriptors = cs->UavMax - cs->UavMin + 1;
		ranges[RangeCount].BaseShaderRegister = cs->UavMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		++RangeCount;
	}
	D3D12_ROOT_PARAMETER1 params[2] = {};
	u32 ParamCount = 0;
	if (RangeCount)
	{
		params[ParamCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[ParamCount].DescriptorTable.NumDescriptorRanges = RangeCount;
		params[ParamCount].DescriptorTable.pDescriptorRanges = &ranges[0];
		params[ParamCount].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		++ParamCount;
	}
	// Sampler
	D3D12_DESCRIPTOR_RANGE1 SamplerRange = {};
	if (cs->NumSamplers)
	{
		SamplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		SamplerRange.NumDescriptors = cs->SamplerMax - cs->SamplerMin + 1;
		SamplerRange.BaseShaderRegister = cs->SamplerMin;
		SamplerRange.RegisterSpace = 0;
		SamplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		SamplerRange.OffsetInDescriptorsFromTableStart = 0;

		params[ParamCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[ParamCount].DescriptorTable.NumDescriptorRanges = 1;
		params[ParamCount].DescriptorTable.pDescriptorRanges = &SamplerRange;
		params[ParamCount].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	
		++ParamCount;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSig = {};
	RootSig.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	RootSig.Desc_1_1.NumParameters = ParamCount;
	RootSig.Desc_1_1.pParameters = params;

	ID3DBlob* SerializedRootSig;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&RootSig, &SerializedRootSig, nullptr); 
	Assert(hr == S_OK, "Failed to create serialized root signature, hr=%x", hr);

	ID3D12RootSignature* Sig;
	hr = device->CreateRootSignature(0,
		SerializedRootSig->GetBufferPointer(),
		SerializedRootSig->GetBufferSize(),
		__uuidof(ID3D12RootSignature),
		(void**)&Sig);
	Assert(hr == S_OK, "failed to create root signature, hr=%x", hr);

	SerializedRootSig->Release();

	cs->GfxRootSig = Sig;
}

/* ------TODO-----------

void CreateInputLayout(ID3D11Device* device, VertexShader* shader, ID3DBlob* blob)
{
	ID3D11ShaderReflection* reflector = shader->Common.Reflector;

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
			blob->GetBufferSize(), &shader->LayoutGfxState);
		Assert(hr == S_OK, "failed to create input layout, hr=%x", hr);
	}
}

*/

void CreateTexture(ID3D12Device* device, Texture* tex)
{
	Assert(tex->GfxState.Resource == nullptr, "Leaking object");

	D3D12_RESOURCE_DESC bufferDesc;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = tex->Size.x;
	bufferDesc.Height = tex->Size.y;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = D3DTextureFormat[(u32)tex->Format];
	bufferDesc.SampleDesc.Count = tex->SampleCount;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	bufferDesc.Flags = RlfToD3d(tex->Flags);
 
	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0;
	uploadHeapProperties.VisibleNodeMask = 0;

	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, 
		D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 
		NULL, IID_PPV_ARGS(&tex->GfxState.Resource));
	CheckHresult(hr, "Texture");

}

void CreateBuffer(ID3D12Device* device, Buffer* buf)
{
	Assert(buf->GfxState.Resource == nullptr, "Leaking object");

	u32 bufSize = buf->ElementSize * buf->ElementCount;

	/* TODO: init data
	void* initData = malloc(bufSize);
	if (buf->InitToZero)
		ZeroMemory(initData, bufSize);
	else if (buf->InitDataSize > 0)
	{
		Assert(buf->InitData, "No data but size is set");
		memcpy(initData, buf->InitData, buf->InitDataSize);
	}
	*/

	D3D12_RESOURCE_DESC bufferDesc;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = bufSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 
	D3D12_HEAP_PROPERTIES uploadHeapProperties;
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 0;
	uploadHeapProperties.VisibleNodeMask = 0;

	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, 
		D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 
		NULL, IID_PPV_ARGS(&buf->GfxState.Resource));
	CheckHresult(hr, "buffer");
}

void CreateView(gfx::Context* ctx, View* v, bool allocate_descriptor)
{
	ID3D12Device* device = ctx->Device;
	ID3D12Resource* res = nullptr;
	if (v->ResourceType == ResourceType::Buffer)
		res = v->Buffer->GfxState.Resource;
	else if (v->ResourceType == ResourceType::Texture)
		res = v->Texture->GfxState.Resource;
	else
		Unimplemented();
	DXGI_FORMAT fmt = D3DTextureFormat[(u32)v->Format];
	if (v->Type == ViewType::SRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC vd;
		vd.Format = fmt;
		vd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (v->ResourceType == ResourceType::Buffer)
		{
			if (v->Buffer->Flags & BufferFlag_Raw)
			{
				vd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				vd.Buffer.FirstElement = 0;
				vd.Buffer.NumElements = v->NumElements > 0 ? v->NumElements :
					v->Buffer->ElementCount;
				vd.Buffer.StructureByteStride = 0;
				vd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			}
			else
			{
				vd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				vd.Buffer.FirstElement = 0;
				vd.Buffer.NumElements = v->NumElements > 0 ? v->NumElements :
					v->Buffer->ElementCount;
				vd.Buffer.StructureByteStride = v->Buffer->ElementSize;
				vd.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			}
		}
		else if (v->ResourceType == ResourceType::Texture)
		{
			vd.ViewDimension = v->Texture->SampleCount > 1 ? 
				D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
			vd.Texture2D.MostDetailedMip = 0;
			vd.Texture2D.MipLevels = (u32)-1;
			vd.Texture2D.PlaneSlice = 0;
			vd.Texture2D.ResourceMinLODClamp = 0.f;
		}
		else 
			Unimplemented();
		if (allocate_descriptor)
			v->SRVGfxState.CpuDescriptor = AllocateCbvSrvUavDescriptor(ctx);
		device->CreateShaderResourceView(res, &vd, v->SRVGfxState.CpuDescriptor);
	}
	else if (v->Type == ViewType::UAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC vd;
		vd.Format = fmt;
		if (v->ResourceType == ResourceType::Buffer)
		{
			vd.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			vd.Buffer.FirstElement = 0;
			vd.Buffer.NumElements = v->NumElements > 0 ? v->NumElements : 
				v->Buffer->ElementCount;
			vd.Buffer.StructureByteStride = v->Buffer->Flags & BufferFlag_Raw ?
				0 : v->Buffer->ElementSize;
			vd.Buffer.CounterOffsetInBytes = 0;
			vd.Buffer.Flags = v->Buffer->Flags & BufferFlag_Raw ? 
				D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
		}
		else if (v->ResourceType == ResourceType::Texture)
		{
			vd.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			vd.Texture2D.MipSlice = 0;
			vd.Texture2D.PlaneSlice = 0;
		}
		else 
			Unimplemented();
		if (allocate_descriptor)
			v->UAVGfxState.CpuDescriptor = AllocateCbvSrvUavDescriptor(ctx);
		device->CreateUnorderedAccessView(res, nullptr, &vd, v->UAVGfxState.CpuDescriptor);
	}
	else if (v->Type == ViewType::RTV)
	{
		D3D12_RENDER_TARGET_VIEW_DESC vd;
		vd.Format = fmt;
		Assert(v->ResourceType == ResourceType::Texture, "Invalid");
		vd.ViewDimension = v->Texture->SampleCount > 1 ? 
			D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
		// NOTE: technically there is a separate field Texture2DMS that should be
		//	filled for that dimension but it has no fields so I'm not bothering.
		vd.Texture2D.MipSlice = 0;
		vd.Texture2D.PlaneSlice = 0;
		if (allocate_descriptor)
			v->RTVGfxState.CpuDescriptor = AllocateRtvDescriptor(ctx);
		device->CreateRenderTargetView(res, &vd, v->RTVGfxState.CpuDescriptor);
	}
	else if (v->Type == ViewType::DSV)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC vd;
		vd.Format = fmt;
		Assert(v->ResourceType == ResourceType::Texture, "Invalid");
		vd.ViewDimension = v->Texture->SampleCount > 1 ? 
			D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
		vd.Flags = D3D12_DSV_FLAG_NONE;
		// NOTE: technically there is a separate field Texture2DMS that should be
		//	filled for that dimension but it has no fields so I'm not bothering.
		vd.Texture2D.MipSlice = 0;
		if (allocate_descriptor)
			v->DSVGfxState.CpuDescriptor = AllocateDsvDescriptor(ctx);
		device->CreateDepthStencilView(res, &vd, v->DSVGfxState.CpuDescriptor);
	}
	else
		Unimplemented();
}

void ResolveBind(Bind& bind, ID3D12ShaderReflection* reflector, const char* path)
{
	D3D12_SHADER_INPUT_BIND_DESC desc;
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

void CopyBindDescriptors(gfx::Context* ctx, ExecuteResources* resources,
	const Dispatch* dc)
{
	const ComputeShader* cs = dc->Shader;
	for (const Bind& bind : dc->Binds)
	{
		u32 DestSlot = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDesc = {};
		switch(bind.Type)
		{
		case BindType::SystemValue:
			switch (bind.SystemBind)
			{
			case SystemValue::BackBuffer:
				if (bind.IsOutput)
				{
					DestSlot = cs->NumCbvs + cs->NumSrvs + (bind.BindIndex - cs->UavMin);
					SrcDesc = resources->MainRtUav.CpuDescriptor;
				}
				else
					Unimplemented();
				break;
			case SystemValue::DefaultDepth:
				Assert(false, "Can not bind DSV to shader");
				break;
			default:
				Unimplemented();
			}
			break;
		case BindType::View:
			if (bind.IsOutput)
			{
				Assert(bind.ViewBind->Type == ViewType::UAV, "mismatched view");
				DestSlot = cs->NumCbvs + cs->NumSrvs + (bind.BindIndex - cs->UavMin);
				SrcDesc = bind.ViewBind->UAVGfxState.CpuDescriptor;
			}
			else
			{
				Assert(bind.ViewBind->Type == ViewType::SRV, "mismatched view");
				DestSlot = cs->NumCbvs + (bind.BindIndex - cs->SrvMin);
				SrcDesc = bind.ViewBind->SRVGfxState.CpuDescriptor;
			}
			break;
		case BindType::Sampler:
		default:
			Unimplemented();
		}
		D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = 
			ctx->ShaderVisDescHeap->GetCPUDescriptorHandleForHeapStart();
		DestDesc.ptr += (dc->CbvSrvUavDescTableStart + DestSlot) * ctx->SrvUavDescSize;
		ctx->Device->CopyDescriptorsSimple(1, DestDesc, SrcDesc,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

u32 AlignU32(u32 val, u32 align)
{
	if (!val)
		return align;
	return (((val-1) / align) + 1) * align;
}

void PrepareConstants(
	gfx::Context* ctx, ID3D12ShaderReflection* reflector, 
	std::vector<ConstantBuffer>& buffers, std::vector<SetConstant>& sets, 
	const char* path)
{
	D3D12_SHADER_DESC sd;
	reflector->GetDesc(&sd);
	HRESULT hr;
	for (u32 i = 0 ; i < sd.ConstantBuffers ; ++i)
	{
		ConstantBuffer cb = {};
		D3D12_SHADER_BUFFER_DESC bd;
		ID3D12ShaderReflectionConstantBuffer* constBuffer = 
			reflector->GetConstantBufferByIndex(i);
		constBuffer->GetDesc(&bd);

		if (bd.Type != D3D_CT_CBUFFER)
			continue;

		cb.Name = bd.Name;
		cb.Size = bd.Size;
		cb.BackingMemory = (u8*)malloc(bd.Size);

		for (u32 j = 0 ; j < bd.Variables ; ++j)
		{
			ID3D12ShaderReflectionVariable* var = constBuffer->GetVariableByIndex(j);
			D3D12_SHADER_VARIABLE_DESC vd;
			var->GetDesc(&vd);

			if (vd.DefaultValue)
				memcpy(cb.BackingMemory+vd.StartOffset, vd.DefaultValue, vd.Size);
			else
				ZeroMemory(cb.BackingMemory+vd.StartOffset, vd.Size);
		}

		u32 alignedSize = AlignU32(bd.Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		D3D12_RESOURCE_DESC constantBufferDesc;
		constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		constantBufferDesc.Alignment = 0;
		constantBufferDesc.Width = alignedSize;
		constantBufferDesc.Height = 1;
		constantBufferDesc.DepthOrArraySize = 1;
		constantBufferDesc.MipLevels = 1;
		constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		constantBufferDesc.SampleDesc.Count = 1;
		constantBufferDesc.SampleDesc.Quality = 0;
		constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	 
		D3D12_HEAP_PROPERTIES uploadHeapProperties;
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadHeapProperties.CreationNodeMask = 0;
		uploadHeapProperties.VisibleNodeMask = 0;

		for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
		{
			hr = ctx->Device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, 
				&constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, 
				IID_PPV_ARGS(&cb.GfxState.Resource[frame]));
			Assert(hr == S_OK, "Failed to create buffer, hr=%x", hr);
			cb.GfxState.Resource[frame]->Map(0, nullptr, (void**)&cb.GfxState.MappedMem[frame]);
			
			cb.GfxState.CbvDescriptor[frame] = AllocateCbvSrvUavDescriptor(ctx);

			D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
			constantBufferViewDesc.BufferLocation = 
				cb.GfxState.Resource[frame]->GetGPUVirtualAddress();
			constantBufferViewDesc.SizeInBytes = alignedSize;
 
			ctx->Device->CreateConstantBufferView(&constantBufferViewDesc, 
				cb.GfxState.CbvDescriptor[frame]);
		}

		for (u32 k = 0 ; k < sd.BoundResources ; ++k)
		{
			D3D12_SHADER_INPUT_BIND_DESC id; 
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
		ID3D12ShaderReflectionVariable* cvar = nullptr;
		const char* cbufname = nullptr;
		for (u32 i = 0 ; i < sd.ConstantBuffers ; ++i)
		{
			D3D12_SHADER_BUFFER_DESC bd;
			ID3D12ShaderReflectionConstantBuffer* buffer = 
				reflector->GetConstantBufferByIndex(i);
			buffer->GetDesc(&bd);
			if (bd.Type != D3D_CT_CBUFFER)
				continue;
			for (u32 j = 0 ; j < bd.Variables ; ++j)
			{
				ID3D12ShaderReflectionVariable* var = buffer->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC vd;
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
		D3D12_SHADER_VARIABLE_DESC vd;
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

		ID3D12ShaderReflectionType* ctype = cvar->GetType();
		D3D12_SHADER_TYPE_DESC td;
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
	ExecuteResources* resources,
	RenderDescription* rd,
	uint2 displaySize,
	const char* workingDirectory,
	ErrorState* errorState)
{
	ID3D12Device* device = ctx->Device;
	gInfoQueue = ctx->InfoQueue;

	std::string dirPath = workingDirectory;

	for (ComputeShader* cs : rd->CShaders)
	{
		ID3DBlob* shaderBlob = CommonCompileShader(&cs->Common, workingDirectory,
			"cs_5_0", errorState);

		HRESULT hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D12ShaderReflection, (void**) &cs->Common.Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		cs->Common.Reflector->GetThreadGroupSize(&cs->ThreadGroupSize.x, 
			&cs->ThreadGroupSize.y,	&cs->ThreadGroupSize.z);

		cs->GfxState = shaderBlob;

		CreateRootSignature(device, cs);

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = cs->GfxRootSig;
		desc.CS.pShaderBytecode = shaderBlob->GetBufferPointer();
		desc.CS.BytecodeLength = shaderBlob->GetBufferSize();
		desc.NodeMask = 0;
		desc.CachedPSO.pCachedBlob = nullptr;
		desc.CachedPSO.CachedBlobSizeInBytes = 0;
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		hr = device->CreateComputePipelineState(&desc, __uuidof(ID3D12PipelineState),
			(void**)&cs->GfxPipeline);
		Assert(hr == S_OK, "Failed to create pipeline state, hr=%x", hr);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		ComputeShader* cs = dc->Shader;
		ID3D12ShaderReflection* reflector = cs->Common.Reflector;
		for (Bind& bind : dc->Binds)
		{
			ResolveBind(bind, reflector, cs->Common.ShaderPath);
		}
		PrepareConstants(ctx, reflector, dc->CBs, dc->Constants, cs->Common.ShaderPath);
	}

	ast::EvaluationContext evCtx;
	evCtx.DisplaySize = displaySize;
	evCtx.Time = 0;
	evCtx.ChangedThisFrameFlags = 0;

	// Size expressions may depend on constants so we need to evaluate them first
	EvaluateConstants(evCtx, rd->Constants);

	for (Buffer* buf : rd->Buffers)
	{
		// Obj initialized buffers don't have expressions.
		if (buf->ElementSizeExpr || buf->ElementCountExpr)
		{
			ast::Result res;
			EvaluateExpression(evCtx, buf->ElementSizeExpr, res, UintType, 
				"Buffer::ElementSize");
			buf->ElementSize = res.Value.UintVal;
			EvaluateExpression(evCtx, buf->ElementCountExpr, res, UintType, 
				"Buffer::ElementCount");
			buf->ElementCount = res.Value.UintVal;
		}

		CreateBuffer(device, buf);
	}

	for (Texture* tex : rd->Textures)
	{
		if (tex->DDSPath)
		{
			Unimplemented();
	/* TODO
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
	*/
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
		CreateView(ctx, v, /*allocate_descriptor:*/true);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		ComputeShader* cs = dc->Shader;
		dc->CbvSrvUavDescTableStart = ctx->ShaderVisDescNextIndex;
		ctx->ShaderVisDescNextIndex += cs->NumCbvs + cs->NumSrvs + cs->NumUavs +
			cs->NumSamplers;
		CopyBindDescriptors(ctx, resources, dc);
	}

/* ------TODO-----------

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



	for (Draw* draw : rd->Draws)
	{
		InitAssert(draw->VShader, "Null vertex shader on draw not permitted.");
		ID3D11ShaderReflection* reflector = draw->VShader->Common.Reflector;
		for (Bind& bind : draw->VSBinds)
		{
			ResolveBind(bind, reflector, draw->VShader->Common.ShaderPath);
		}
		PrepareConstants(ctx, reflector, draw->VSCBs, draw->VSConstants,
			draw->VShader->Common.ShaderPath);
		if (draw->PShader)
		{
			reflector = draw->PShader->Common.Reflector;
			for (Bind& bind : draw->PSBinds)
			{
				ResolveBind(bind, reflector, draw->PShader->Common.ShaderPath);
			}
			PrepareConstants(ctx, reflector, draw->PSCBs, draw->PSConstants,
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
*/
}

void ReleaseD3D(
	gfx::Context* ctx,
	RenderDescription* rd)
{
	// Reset descriptor heaps
	ctx->SrvUavDescNextIndex = gfx::Context::NUM_RESERVED_SRV_UAV_SLOTS;
	ctx->RtvDescNextIndex = gfx::Context::NUM_RESERVED_RTV_SLOTS;
	ctx->DsvDescNextIndex = gfx::Context::NUM_RESERVED_DSV_SLOTS;
	ctx->ShaderVisDescNextIndex = gfx::Context::NUM_RESERVED_SHADER_VIS_SLOTS;

	for (ComputeShader* cs : rd->CShaders)
	{
		SafeRelease(cs->GfxPipeline);
		SafeRelease(cs->GfxRootSig);
		SafeRelease(cs->Common.Reflector);
		SafeRelease(cs->GfxState);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		for (ConstantBuffer& cb : dc->CBs)
		{
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				cb.GfxState.Resource[frame]->Unmap(0, nullptr);
				SafeRelease(cb.GfxState.Resource[frame]);
			}
			free(cb.BackingMemory);
		}
	}

	for (Buffer* buf : rd->Buffers)
	{
		SafeRelease(buf->GfxState.Resource);
	}

	for (Texture* tex : rd->Textures)
	{
		SafeRelease(tex->GfxState.Resource);
	}

/* ------TODO-----------

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
*/
}

void HandleTextureParametersChanged(
	RenderDescription* rd,
	ExecuteContext* ec,
	ErrorState* errorState)
{
	gfx::Context* ctx = ec->GfxCtx;
	ID3D12Device* device = ctx->Device;
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

				SafeRelease(tex->GfxState.Resource);
				
				CreateTexture(device, tex);

				// Find any views that use this texture and recreate them. 
				for (View* view : tex->Views)
				{
					if (view->ResourceType == ResourceType::Texture && 
						view->Texture == tex)
					{
						CreateView(ctx, view, /*allocate_descriptor*/false);
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

				SafeRelease(buf->GfxState.Resource);
				
				CreateBuffer(device, buf);

				// Find any views that use this buffer and recreate them. 
				for (View* view : buf->Views)
				{
					if (view->ResourceType == ResourceType::Buffer && 
						view->Buffer == buf)
					{
						CreateView(ctx, view, /*allocate_descriptor:*/false);
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

	// TODO: do draws too
	for (Dispatch* dc : rd->Dispatches)
	{
		CopyBindDescriptors(ctx, &ec->Res, dc);
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
	gfx::Context* ctx = ec->GfxCtx;
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
		u32 frame = ctx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT;
		memcpy(buf.GfxState.MappedMem[frame], buf.BackingMemory, buf.Size);
	}
}

void ExecuteDispatch(
	Dispatch* dc,
	ExecuteContext* ec)
{
	ComputeShader* cs = dc->Shader;
	ExecuteSetConstants(ec, dc->Constants, dc->CBs);

	// Update cbvs in descriptor table since we use a different one each frame.
	for (const ConstantBuffer& buf : dc->CBs)
	{
		u32 frame = ec->GfxCtx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT;
		u32 DestSlot = buf.Slot - cs->CbvMin;

		D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = 
			ec->GfxCtx->ShaderVisDescHeap->GetCPUDescriptorHandleForHeapStart();
		DestDesc.ptr += (dc->CbvSrvUavDescTableStart + DestSlot) * ec->GfxCtx->SrvUavDescSize;
		ec->GfxCtx->Device->CopyDescriptorsSimple(1, DestDesc, buf.GfxState.CbvDescriptor[frame],
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	ID3D12GraphicsCommandList* cl = ec->GfxCtx->CommandList;
	cl->SetPipelineState(cs->GfxPipeline);
	cl->SetComputeRootSignature(cs->GfxRootSig);
	D3D12_GPU_DESCRIPTOR_HANDLE table_handle = 
		ec->GfxCtx->ShaderVisDescHeap->GetGPUDescriptorHandleForHeapStart();
	table_handle.ptr += dc->CbvSrvUavDescTableStart * ec->GfxCtx->SrvUavDescSize;
	// TODO: check if cbv/srv/uav table exists
	cl->SetComputeRootDescriptorTable(0, table_handle);
	// TODO: check and bind sampler table if it exists

	if (dc->IndirectArgs)
	{
		// TODO: implement indirect args
		// ctx->DispatchIndirect(dc->IndirectArgs->GfxState, dc->IndirectArgsOffset);
		Unimplemented();
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

		cl->Dispatch(groups.x, groups.y, groups.z);
	}
}

/* ------TODO-----------
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
	for (Bind& bind : draw->PSBinds)
	{
		switch (bind.Type)
		{
		case BindType::View:
		{
			ExecuteAssert(bind.ViewBind->Type == ViewType::SRV, 
				"UAVs are not supported in pixel shaders.");
			ctx->PSSetShaderResources(bind.BindIndex, 1, &bind.ViewBind->SRVGfxState);
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
				rtViews[rtCount] = ec->MainRtv;
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
				dsView = ec->DefaultDepthView;
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
	ctx->OMSetRenderTargets(8, rtViews, dsView);
	ctx->RSSetViewports(8, vp);
	ctx->IASetPrimitiveTopology(RlfToD3d(draw->Topology));
	ctx->RSSetState(draw->RState ? draw->RState->GfxState : DefaultRasterizerState);
	ctx->OMSetDepthStencilState(draw->DSState ? draw->DSState->GfxState : nullptr,
		draw->StencilRef);
	ctx->OMSetBlendState(draw->BlendGfxState, nullptr, 0xffffffff);
	u32 offset = 0;
	Buffer* vb = draw->VertexBuffer;
	Buffer* ib = draw->IndexBuffer;
	if (vb)
		ctx->IASetVertexBuffers(0, 1, &vb->GfxState, &vb->ElementSize, 
			&offset);
	if (ib)
		ctx->IASetIndexBuffer(ib->GfxState, ib->ElementSize == 2 ? 
			DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);

	if (draw->InstancedIndirectArgs)
		ctx->DrawInstancedIndirect(draw->InstancedIndirectArgs->GfxState,
			draw->IndirectArgsOffset);
	else if (draw->IndexedInstancedIndirectArgs)
		ctx->DrawIndexedInstancedIndirect(draw->InstancedIndirectArgs->GfxState,
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
*/

void _Execute(
	ExecuteContext* ec,
	RenderDescription* rd)
{
	gfx::Context* ctx = ec->GfxCtx;

	EvaluateConstants(ec->EvCtx, rd->Constants);

	// Clear state so we aren't polluted by previous program drawing or previous 
	//	execution. 
	ctx->CommandList->ClearState(nullptr);
	ctx->CommandList->SetDescriptorHeaps(1, &ctx->ShaderVisDescHeap);

	for (Pass pass : rd->Passes)
	{
		if (pass.Type == PassType::Dispatch)
		{
			ExecuteDispatch(pass.Dispatch, ec);
		}
		/* ------TODO-----------
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
		*/
		else
		{
			Unimplemented();
		}

		// Clear state after execution so we don't pollute the rest of program drawing. 
		ctx->CommandList->ClearState(nullptr);
		ctx->CommandList->SetDescriptorHeaps(1, &ctx->ShaderVisDescHeap);
	}
}

#undef ExecuteAssert
#undef SafeRelease

} // namespace rlf
