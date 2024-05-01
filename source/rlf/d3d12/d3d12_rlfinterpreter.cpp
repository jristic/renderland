
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


D3D12_FILTER RlfToD3d(FilterMode fm)
{
	if (fm.Min == Filter::Aniso || fm.Mag == Filter::Aniso ||
		fm.Mip == Filter::Aniso)
	{
		return D3D12_FILTER_ANISOTROPIC;
	}

	static D3D12_FILTER filters[] = {
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
		D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
		D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
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

D3D12_TEXTURE_ADDRESS_MODE RlfToD3d(AddressMode m)
{
	Assert(m != AddressMode::Invalid, "Invalid");
	static D3D12_TEXTURE_ADDRESS_MODE modes[] = {
		(D3D12_TEXTURE_ADDRESS_MODE)0, //invalid
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
	};
	return modes[(u32)m];
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE RlfToD3d_TopoType(Topology topo)
{
	Assert(topo != Topology::Invalid, "Invalid");
	static D3D12_PRIMITIVE_TOPOLOGY_TYPE topos[] = {
		(D3D12_PRIMITIVE_TOPOLOGY_TYPE)0, //invalid
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	};
	return topos[(u32)topo];
}

D3D_PRIMITIVE_TOPOLOGY RlfToD3d_Topo(Topology topo)
{
	Assert(topo != Topology::Invalid, "Invalid");
	static D3D_PRIMITIVE_TOPOLOGY topos[] = {
		(D3D_PRIMITIVE_TOPOLOGY)0, //invalid
		D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
		D3D_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	};
	return topos[(u32)topo];
}

D3D12_CULL_MODE RlfToD3d(CullMode cm)
{
	Assert(cm != CullMode::Invalid, "Invalid");
	static D3D12_CULL_MODE cms[] = {
		(D3D12_CULL_MODE)0, //invalid
		D3D12_CULL_MODE_NONE,
		D3D12_CULL_MODE_FRONT,
		D3D12_CULL_MODE_BACK,
	};
	return cms[(u32)cm];
}

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

D3D12_COMPARISON_FUNC RlfToD3d(ComparisonFunc cf)
{
	Assert(cf != ComparisonFunc::Invalid, "Invalid");
	static D3D12_COMPARISON_FUNC cfs[] = {
		(D3D12_COMPARISON_FUNC)0, //invalid
		D3D12_COMPARISON_FUNC_NEVER,
		D3D12_COMPARISON_FUNC_LESS,
		D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER,
		D3D12_COMPARISON_FUNC_NOT_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER_EQUAL,
		D3D12_COMPARISON_FUNC_ALWAYS,
	};
	return cfs[(u32)cf];
}

D3D12_STENCIL_OP RlfToD3d(StencilOp so)
{
	Assert(so != StencilOp::Invalid, "Invalid");
	static D3D12_STENCIL_OP sos[] = {
		(D3D12_STENCIL_OP)0, //invalid
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_ZERO,
		D3D12_STENCIL_OP_REPLACE,
		D3D12_STENCIL_OP_INCR_SAT,
		D3D12_STENCIL_OP_DECR_SAT,
		D3D12_STENCIL_OP_INVERT,
		D3D12_STENCIL_OP_INCR,
		D3D12_STENCIL_OP_DECR
	};
	return sos[(u32)so];
}

D3D12_DEPTH_STENCILOP_DESC RlfToD3d(StencilOpDesc sod)
{
	D3D12_DEPTH_STENCILOP_DESC desc = {};
	desc.StencilFailOp = RlfToD3d(sod.StencilFailOp);
	desc.StencilDepthFailOp = RlfToD3d(sod.StencilDepthFailOp);
	desc.StencilPassOp = RlfToD3d(sod.StencilPassOp);
	desc.StencilFunc = RlfToD3d(sod.StencilFunc);
	return desc;
}

D3D12_BLEND RlfToD3d(Blend b)
{
	Assert(b != Blend::Invalid, "Invalid");
	static D3D12_BLEND bs[] = {
		(D3D12_BLEND)0, //invalid
		D3D12_BLEND_ZERO,
		D3D12_BLEND_ONE,
		D3D12_BLEND_SRC_COLOR,
		D3D12_BLEND_INV_SRC_COLOR,
		D3D12_BLEND_SRC_ALPHA,
		D3D12_BLEND_INV_SRC_ALPHA,
		D3D12_BLEND_DEST_ALPHA,
		D3D12_BLEND_INV_DEST_ALPHA,
		D3D12_BLEND_DEST_COLOR,
		D3D12_BLEND_INV_DEST_COLOR,
		D3D12_BLEND_SRC_ALPHA_SAT,
		D3D12_BLEND_BLEND_FACTOR,
		D3D12_BLEND_INV_BLEND_FACTOR,
		D3D12_BLEND_SRC1_COLOR,
		D3D12_BLEND_INV_SRC1_COLOR,
		D3D12_BLEND_SRC1_ALPHA,
		D3D12_BLEND_INV_SRC1_ALPHA,
	};
	return bs[(u32)b];
}

D3D12_BLEND_OP RlfToD3d(BlendOp op)
{
	Assert(op != BlendOp::Invalid, "Invalid");
	static D3D12_BLEND_OP ops[] = {
		(D3D12_BLEND_OP)0, //invalid
		D3D12_BLEND_OP_ADD,
		D3D12_BLEND_OP_SUBTRACT,
		D3D12_BLEND_OP_REV_SUBTRACT,
		D3D12_BLEND_OP_MIN,
		D3D12_BLEND_OP_MAX,
	};
	return ops[(u32)op];
}

D3D12_INPUT_CLASSIFICATION RlfToD3d(InputClassification ic)
{
	Assert(ic != InputClassification::Invalid, "Invalid");
	static D3D12_INPUT_CLASSIFICATION ics[] = {
		(D3D12_INPUT_CLASSIFICATION)0, //invalid
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ,
		D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
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
				path, request->StructName);
			request->Size = search->second;
		}
	}

	free(shaderBuffer);

	return shaderBlob;
}

void GatherBinds(gfx::BindInfo* bi, ID3D12ShaderReflection* reflector)
{
	bi->NumCbvs = 0;
	bi->NumSrvs = 0;
	bi->NumUavs = 0;
	bi->NumSamplers = 0;

	bi->CbvMin = U32_MAX;
	bi->CbvMax = 0;
	bi->SrvMin = U32_MAX;
	bi->SrvMax = 0;
	bi->UavMin = U32_MAX;
	bi->UavMax = 0;
	bi->SamplerMin = U32_MAX;
	bi->SamplerMax = 0;

	D3D12_SHADER_DESC ShaderDesc = {};
	reflector->GetDesc(&ShaderDesc);

	for (u32 i = 0 ; i < ShaderDesc.BoundResources ; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC input = {};
		reflector->GetResourceBindingDesc(i, &input);

		InitAssert(input.BindCount == 1, "Multi-bind-point resources are unsupported.");
		InitAssert(input.Space == 0, "Non-zero register spaces are unsupported.");

		switch(input.Type)
		{
		case D3D_SIT_CBUFFER:
			bi->CbvMin = min(bi->CbvMin, input.BindPoint);
			bi->CbvMax = max(bi->CbvMax, input.BindPoint);
			bi->NumCbvs = bi->CbvMax - bi->CbvMin + 1;
			break;
		case D3D_SIT_TBUFFER:
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			bi->SrvMin = min(bi->SrvMin, input.BindPoint);
			bi->SrvMax = max(bi->SrvMax, input.BindPoint);
			bi->NumSrvs = bi->SrvMax - bi->SrvMin + 1;
			break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			bi->UavMin = min(bi->UavMin, input.BindPoint);
			bi->UavMax = max(bi->UavMax, input.BindPoint);
			bi->NumUavs = bi->UavMax - bi->UavMin + 1;
			break;
		case D3D_SIT_SAMPLER:
			bi->SamplerMin = min(bi->SamplerMin, input.BindPoint);
			bi->SamplerMax = max(bi->SamplerMax, input.BindPoint);
			bi->NumSamplers = bi->SamplerMax - bi->SamplerMin + 1;
			break;
		case D3D_SIT_RTACCELERATIONSTRUCTURE:
		case D3D_SIT_UAV_FEEDBACKTEXTURE:
		default:
			Unimplemented();
		}
	}
}

void GenerateRangesParameters(gfx::BindInfo* bi, D3D12_SHADER_VISIBILITY visiblity,
	D3D12_DESCRIPTOR_RANGE1 ranges[], D3D12_ROOT_PARAMETER1 params[], 
	u32* range_count, u32* param_count)
{
	u32& RangeCount = *range_count;
	u32& ParamCount = *param_count;
	u32 range_start = RangeCount;
	// CBVs
	if (bi->NumCbvs)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[RangeCount].NumDescriptors = bi->CbvMax - bi->CbvMin + 1;
		ranges[RangeCount].BaseShaderRegister = bi->CbvMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 0;
		++RangeCount;
	}
	// SRVs
	if (bi->NumSrvs)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[RangeCount].NumDescriptors = bi->SrvMax - bi->SrvMin + 1;
		ranges[RangeCount].BaseShaderRegister = bi->SrvMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		++RangeCount;
	}
	// UAVs
	if (bi->NumUavs)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[RangeCount].NumDescriptors = bi->UavMax - bi->UavMin + 1;
		ranges[RangeCount].BaseShaderRegister = bi->UavMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 
			D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		++RangeCount;
	}
	if (bi->NumCbvs + bi->NumSrvs + bi->NumUavs)
	{
		params[ParamCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[ParamCount].DescriptorTable.NumDescriptorRanges = RangeCount-range_start;
		params[ParamCount].DescriptorTable.pDescriptorRanges = &ranges[range_start];
		params[ParamCount].ShaderVisibility = visiblity;
		++ParamCount;
	}
	// Sampler
	if (bi->NumSamplers)
	{
		ranges[RangeCount].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		ranges[RangeCount].NumDescriptors = bi->SamplerMax - bi->SamplerMin + 1;
		ranges[RangeCount].BaseShaderRegister = bi->SamplerMin;
		ranges[RangeCount].RegisterSpace = 0;
		ranges[RangeCount].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[RangeCount].OffsetInDescriptorsFromTableStart = 0;
		++RangeCount;

		params[ParamCount].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[ParamCount].DescriptorTable.NumDescriptorRanges = 1;
		params[ParamCount].DescriptorTable.pDescriptorRanges = &ranges[RangeCount-1];
		params[ParamCount].ShaderVisibility = visiblity;	
		++ParamCount;
	}
}

void CreateRootSignature(ID3D12Device* device, ComputeShader* c)
{
	gfx::ComputeShader* cs = &c->GfxState;
	ID3D12ShaderReflection* reflector = c->Common.Reflector;

	GatherBinds(&cs->BI, reflector);

	D3D12_DESCRIPTOR_RANGE1 ranges[4];
	D3D12_ROOT_PARAMETER1 params[2];
	u32 RangeCount = 0;
	u32 ParamCount = 0;
	GenerateRangesParameters(&cs->BI, D3D12_SHADER_VISIBILITY_ALL, ranges, params, 
		&RangeCount, &ParamCount);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSig = {};
	RootSig.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	RootSig.Desc_1_1.NumParameters = ParamCount;
	RootSig.Desc_1_1.pParameters = params;
	RootSig.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

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

	cs->RootSig = Sig;
}

void CreateRootSignature(ID3D12Device* device, Draw* d)
{
	VertexShader* vs = d->VShader;
	PixelShader* ps = d->PShader;

	D3D12_DESCRIPTOR_RANGE1 ranges[8];
	D3D12_ROOT_PARAMETER1 params[4];
	u32 RangeCount = 0;
	u32 ParamCount = 0;
	GenerateRangesParameters(&vs->GfxState.BI, D3D12_SHADER_VISIBILITY_VERTEX, ranges, 
		params, &RangeCount, &ParamCount);
	if (ps)
		GenerateRangesParameters(&ps->GfxState.BI, D3D12_SHADER_VISIBILITY_PIXEL, ranges, 
			params, &RangeCount, &ParamCount);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSig = {};
	RootSig.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	RootSig.Desc_1_1.NumParameters = ParamCount;
	RootSig.Desc_1_1.pParameters = params;
	RootSig.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	{
		D3D12_SHADER_DESC shaderDesc;
		vs->Common.Reflector->GetDesc( &shaderDesc );
		if (shaderDesc.InputParameters > 0)
			RootSig.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

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

	d->GfxState.RootSig = Sig;
}

void CreateInputLayout(VertexShader* shader, std::vector<D3D12_INPUT_ELEMENT_DESC>& descs)
{
	if (shader->InputLayout.Count == 0)
	{
		ID3D12ShaderReflection* reflector = shader->Common.Reflector;
		// Get shader info
		D3D12_SHADER_DESC shaderDesc;
		reflector->GetDesc( &shaderDesc );

		for ( u32 i = 0 ; i < shaderDesc.InputParameters ; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
			reflector->GetInputParameterDesc(i, &paramDesc );

			if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED)
				continue;

			// fill out input element desc
			D3D12_INPUT_ELEMENT_DESC elementDesc;
			elementDesc.SemanticName = paramDesc.SemanticName;
			elementDesc.SemanticIndex = paramDesc.SemanticIndex;
			elementDesc.InputSlot = 0;
			elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
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

			descs.push_back(elementDesc);
		}
	}
	else
	{
		descs.resize(shader->InputLayout.Count);
		for (u32 i = 0 ; i < shader->InputLayout.Count ; ++i)
		{
			D3D12_INPUT_ELEMENT_DESC& out = descs[i];
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
}

void CreatePipelineState(ID3D12Device* device, Draw* d)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = d->GfxState.RootSig;
	desc.VS.pShaderBytecode = d->VShader->GfxState.Blob->GetBufferPointer();
	desc.VS.BytecodeLength = d->VShader->GfxState.Blob->GetBufferSize();
	if (d->PShader)
	{
		desc.PS.pShaderBytecode = d->PShader->GfxState.Blob->GetBufferPointer();
		desc.PS.BytecodeLength = d->PShader->GfxState.Blob->GetBufferSize();
	}

	u32 blendCount = (u32)d->BlendStates.size();
	desc.BlendState.AlphaToCoverageEnable = false;
	desc.BlendState.IndependentBlendEnable = blendCount > 1;
	if (blendCount > 0)
	{
		InitAssert(blendCount <= 8, "Too many blend states on draw, max is 8.");
		for (u32 i = 0 ; i < blendCount ; ++i)
		{
			BlendState* blend = d->BlendStates[i];
			desc.BlendState.RenderTarget[i].BlendEnable = blend->Enable;
			desc.BlendState.RenderTarget[i].LogicOpEnable = false;
			desc.BlendState.RenderTarget[i].SrcBlend = RlfToD3d(blend->Src);
			desc.BlendState.RenderTarget[i].DestBlend = RlfToD3d(blend->Dest);
			desc.BlendState.RenderTarget[i].BlendOp = RlfToD3d(blend->Op);
			desc.BlendState.RenderTarget[i].SrcBlendAlpha = RlfToD3d(blend->SrcAlpha);
			desc.BlendState.RenderTarget[i].DestBlendAlpha = RlfToD3d(blend->DestAlpha);
			desc.BlendState.RenderTarget[i].BlendOpAlpha = RlfToD3d(blend->OpAlpha);
			desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.BlendState.RenderTarget[i].RenderTargetWriteMask = 
				blend->RenderTargetWriteMask;
		}
	}
	// set defaults to the remainder
	for (u32 i = blendCount ; i < 8 ; ++i)
	{
		desc.BlendState.RenderTarget[i].BlendEnable = false;
		desc.BlendState.RenderTarget[i].LogicOpEnable = false;
		desc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		desc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		desc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		desc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		desc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		desc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		desc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	desc.SampleMask = 0xffffffff;

	RasterizerState* rs = d->RState;
	if (rs)
	{
		desc.RasterizerState.FillMode = rs->Fill ? D3D12_FILL_MODE_SOLID : 
			D3D12_FILL_MODE_WIREFRAME;
		desc.RasterizerState.CullMode = RlfToD3d(rs->CullMode);
		desc.RasterizerState.FrontCounterClockwise = rs->FrontCCW;
		desc.RasterizerState.DepthBias = rs->DepthBias;
		desc.RasterizerState.SlopeScaledDepthBias = rs->SlopeScaledDepthBias;
		desc.RasterizerState.DepthBiasClamp = rs->DepthBiasClamp;
		desc.RasterizerState.DepthClipEnable = rs->DepthClipEnable;
		// TODO: scissor doesn't exist here in dx12, what do
		// desc.RasterizerState.ScissorEnable = rs->ScissorEnable;
		desc.RasterizerState.MultisampleEnable = rs->MultisampleEnable;
		desc.RasterizerState.AntialiasedLineEnable = rs->AntialiasedLineEnable;
	}
	else
	{
		// NOTE: I am using some different defaults than d3d12 normally provides, 
		//	because I do not like their choices. 
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;  
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; 
		desc.RasterizerState.FrontCounterClockwise = FALSE;  
		desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;  
		desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;  
		desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;  
		desc.RasterizerState.DepthClipEnable = TRUE;  
		desc.RasterizerState.MultisampleEnable = FALSE;  
		desc.RasterizerState.AntialiasedLineEnable = FALSE;  
		desc.RasterizerState.ForcedSampleCount = 0;  
		desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;  
	}

	DepthStencilState* dss = d->DSState;
	if (dss)
	{
		desc.DepthStencilState.DepthEnable = dss->DepthEnable && d->DepthStencil.size() > 0;
		desc.DepthStencilState.DepthWriteMask = dss->DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL :
			D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthStencilState.DepthFunc = RlfToD3d(dss->DepthFunc);
		desc.DepthStencilState.StencilEnable = dss->StencilEnable;
		desc.DepthStencilState.StencilReadMask = dss->StencilReadMask;
		desc.DepthStencilState.StencilWriteMask = dss->StencilWriteMask;
		desc.DepthStencilState.FrontFace = RlfToD3d(dss->FrontFace);
		desc.DepthStencilState.BackFace = RlfToD3d(dss->BackFace);
	}
	else
	{
		desc.DepthStencilState.DepthEnable = d->DepthStencil.size() > 0;
		desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.DepthStencilState.StencilEnable = FALSE;
		desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =  { D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		desc.DepthStencilState.FrontFace = defaultStencilOp;
		desc.DepthStencilState.BackFace = defaultStencilOp;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDescs;
	CreateInputLayout(d->VShader, inputLayoutDescs);
	desc.InputLayout.pInputElementDescs = inputLayoutDescs.data();
	desc.InputLayout.NumElements = (u32)inputLayoutDescs.size();

	desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	desc.PrimitiveTopologyType = RlfToD3d_TopoType(d->Topology);

	u32 sample_count = 1;

	desc.NumRenderTargets = (u32)d->RenderTargets.size();
	for (u32 i = 0 ; i < d->RenderTargets.size() ; ++i)
	{
		const TextureTarget& t = d->RenderTargets[i];
		if (t.IsSystem)
		{
			Assert(t.System == SystemValue::BackBuffer, "unsupported");
			desc.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else
		{
			Assert(t.View->Type == ViewType::RTV, "Invalid");
			Assert(t.View->ResourceType == ResourceType::Texture, "Invalid");
			DXGI_FORMAT fmt = t.View->Format != TextureFormat::Invalid ?
				D3DTextureFormat[(u32)t.View->Format] :
				D3DTextureFormat[(u32)t.View->Texture->Format];
			desc.RTVFormats[i] = fmt;
			sample_count = t.View->Texture->SampleCount;
		}
	}

	if (d->DepthStencil.size() > 0)
	{
		Assert(d->DepthStencil.size() == 1, "only one supported");
		TextureTarget t = d->DepthStencil[0];
		if (t.IsSystem)
		{
			if (t.System == SystemValue::DefaultDepth)
			{
				desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			}
			else
				Unimplemented();
		}
		else
		{
			Assert(t.View->Type == ViewType::DSV, "Invalid");
			Assert(t.View->ResourceType == ResourceType::Texture, "Invalid");
			DXGI_FORMAT fmt = t.View->Format != rlf::TextureFormat::Invalid ?
				D3DTextureFormat[(u32)t.View->Format] :
				D3DTextureFormat[(u32)t.View->Texture->Format];
			desc.DSVFormat = fmt;
			sample_count = t.View->Texture->SampleCount;
		}
	}
	else
	{
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	}

	desc.SampleDesc.Count = sample_count;
	desc.SampleDesc.Quality = 0;

	desc.NodeMask = 0;

	desc.CachedPSO.pCachedBlob = nullptr;
	desc.CachedPSO.CachedBlobSizeInBytes = 0;

	HRESULT hr = device->CreateGraphicsPipelineState(&desc, 
		IID_PPV_ARGS(&d->GfxState.Pipeline));
	Assert(hr == S_OK, "failed to create graphics pipeline state, hr=%x", hr);
}

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

	tex->GfxState.State = D3D12_RESOURCE_STATE_GENERIC_READ;
}

void CreateBuffer(gfx::Context* ctx, Buffer* buf)
{
	ID3D12Device* device = ctx->Device;
	Assert(buf->GfxState.Resource == nullptr, "Leaking object");

	u32 bufSize = buf->ElementSize * buf->ElementCount;

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
 
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;

	bool needs_upload = buf->InitToZero || buf->InitDataSize > 0;
	D3D12_RESOURCE_STATES start_state = needs_upload ? D3D12_RESOURCE_STATE_COPY_DEST : 
		D3D12_RESOURCE_STATE_GENERIC_READ;

	HRESULT hr = device->CreateCommittedResource(&heapProperties, 
		D3D12_HEAP_FLAG_NONE, &bufferDesc, start_state,
		NULL, IID_PPV_ARGS(&buf->GfxState.Resource));
	CheckHresult(hr, "buffer");

	buf->GfxState.State = start_state;

	if (!needs_upload)
		return;

	Assert(bufSize < gfx::Context::UPLOAD_BUFFER_SIZE, "upload data too large.");

	gfx::Context::Frame* frameCtx = 
		&ctx->FrameContexts[ctx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT];
	ctx->UploadCommandList->Reset(frameCtx->CommandAllocator, nullptr);

	if (buf->InitToZero)
	{
		ZeroMemory(ctx->UploadBufferMem, bufSize);
	}
	else if (buf->InitDataSize > 0)
	{
		Assert(buf->InitData, "No data but size is set");
		memcpy(ctx->UploadBufferMem, buf->InitData, buf->InitDataSize);
	}

	ctx->UploadCommandList->CopyBufferRegion(buf->GfxState.Resource, 0, 
		ctx->UploadBufferResource, 0, bufSize);
	ctx->UploadCommandList->Close();
	ctx->CommandQueue->ExecuteCommandLists(1, 
		(ID3D12CommandList* const*)&ctx->UploadCommandList);

	u64 fenceValue = ctx->FenceLastSignaledValue + 1;
	ctx->CommandQueue->Signal(ctx->Fence, fenceValue);
	ctx->FenceLastSignaledValue = fenceValue;

	frameCtx->FenceValue = 0;
	ctx->Fence->SetEventOnCompletion(fenceValue, ctx->FenceEvent);
	WaitForSingleObject(ctx->FenceEvent, INFINITE);
}

u32 GetPlaneSlice(TextureFormat fmt)
{
	switch (fmt)
	{
	case TextureFormat::X24_TYPELESS_G8_UINT:
		return 1;
	default:
		return 0;
	}
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
			vd.Texture2D.PlaneSlice = GetPlaneSlice(v->Format);
			vd.Texture2D.ResourceMinLODClamp = 0.f;
		}
		else 
			Unimplemented();
		if (allocate_descriptor)
			v->SRVGfxState = AllocateDescriptor(&ctx->CbvSrvUavCreationHeap);
		device->CreateShaderResourceView(res, &vd, v->SRVGfxState);
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
			vd.Buffer.StructureByteStride = v->Buffer->Flags & BufferFlag_Structured ?
				v->Buffer->ElementSize : 0;
			vd.Buffer.CounterOffsetInBytes = 0;
			vd.Buffer.Flags = v->Buffer->Flags & BufferFlag_Raw ? 
				D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;
		}
		else if (v->ResourceType == ResourceType::Texture)
		{
			vd.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			vd.Texture2D.MipSlice = 0;
			vd.Texture2D.PlaneSlice = GetPlaneSlice(v->Format);
		}
		else 
			Unimplemented();
		if (allocate_descriptor)
			v->UAVGfxState = AllocateDescriptor(&ctx->CbvSrvUavCreationHeap);
		device->CreateUnorderedAccessView(res, nullptr, &vd, v->UAVGfxState);
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
		vd.Texture2D.PlaneSlice = GetPlaneSlice(v->Format);
		if (allocate_descriptor)
			v->RTVGfxState = AllocateDescriptor(&ctx->RtvHeap);
		device->CreateRenderTargetView(res, &vd, v->RTVGfxState);
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
			v->DSVGfxState = AllocateDescriptor(&ctx->DsvHeap);
		device->CreateDepthStencilView(res, &vd, v->DSVGfxState);
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

void ApplyNullDescriptors(gfx::Context* ctx, const gfx::BindInfo* bi, 
	const gfx::DescriptorTable* table, ID3D12ShaderReflection* reflector)
{
	D3D12_SHADER_DESC ShaderDesc = {};
	reflector->GetDesc(&ShaderDesc);

	for (u32 i = 0 ; i < ShaderDesc.BoundResources ; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC input = {};
		reflector->GetResourceBindingDesc(i, &input);

		// all constant buffers are handled separately from other binds, so 
		// 	we don't try to null them here.
		if (input.Type == D3D_SIT_CBUFFER)
			continue;

		u8 Dimension = 0;
		switch(input.Dimension)
		{
		case D3D_SRV_DIMENSION_BUFFER:
		case D3D_SRV_DIMENSION_TEXTURE1D:
		case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
		case D3D_SRV_DIMENSION_TEXTURE2D:
		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
		case D3D_SRV_DIMENSION_TEXTURE2DMS:
		case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
		case D3D_SRV_DIMENSION_TEXTURE3D:
		case D3D_SRV_DIMENSION_TEXTURECUBE:
		case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
			Dimension = (u8)input.Dimension;
			break;
		case D3D_SRV_DIMENSION_UNKNOWN:
			Assert(input.Type == D3D_SIT_SAMPLER, "unexpected conditions");
			break;
		case D3D_SRV_DIMENSION_BUFFEREX:
		default:
			Unimplemented();
		}

		switch(input.Type)
		{
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8_UINT;
			desc.ViewDimension = (D3D12_SRV_DIMENSION)Dimension;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			u32 DestSlot = bi->NumCbvs + (input.BindPoint - bi->SrvMin);
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = GetCPUDescriptor(&ctx->CbvSrvUavHeap,
					table->CbvSrvUavDescTableStart[frame] + DestSlot);
				ctx->Device->CreateShaderResourceView(nullptr, &desc, DestDesc);
			}
			break;
		}
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		{
			Assert(Dimension <= D3D12_UAV_DIMENSION_TEXTURE3D, "invalid UAV dimension");
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8_UINT;
			desc.ViewDimension = (D3D12_UAV_DIMENSION)Dimension;
			u32 DestSlot = bi->NumCbvs + bi->NumSrvs + (input.BindPoint - bi->UavMin);
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = GetCPUDescriptor(&ctx->CbvSrvUavHeap,
					table->CbvSrvUavDescTableStart[frame] + DestSlot);
				ctx->Device->CreateUnorderedAccessView(nullptr, nullptr, &desc, DestDesc);
			}
			break;
		}
		case D3D_SIT_SAMPLER:
		{
			D3D12_SAMPLER_DESC desc = {};
			desc.AddressU = desc.AddressV = desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = GetCPUDescriptor(&ctx->SamplerHeap,
					table->SamplerDescTableStart[frame] + input.BindPoint - bi->SamplerMin);
				ctx->Device->CreateSampler(&desc, DestDesc);
			}
			break;
		}
		case D3D_SIT_CBUFFER:
		case D3D_SIT_TBUFFER:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		case D3D_SIT_RTACCELERATIONSTRUCTURE:
		case D3D_SIT_UAV_FEEDBACKTEXTURE:
		default:
			Unimplemented();
		}
	}
}

void CopyBindDescriptors(gfx::Context* ctx, ExecuteResources* resources,
	const gfx::BindInfo* bi, const gfx::DescriptorTable* table, 
	const std::vector<Bind>& binds)
{
	for (const Bind& bind : binds)
	{
		u32 DestSlot = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDesc = {};
		bool sampler = false;
		switch(bind.Type)
		{
		case BindType::SystemValue:
			switch (bind.SystemBind)
			{
			case SystemValue::BackBuffer:
				if (bind.IsOutput)
				{
					DestSlot = bi->NumCbvs + bi->NumSrvs + (bind.BindIndex - bi->UavMin);
					SrcDesc = resources->MainRtUav;
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
				DestSlot = bi->NumCbvs + bi->NumSrvs + (bind.BindIndex - bi->UavMin);
				SrcDesc = bind.ViewBind->UAVGfxState;
			}
			else
			{
				Assert(bind.ViewBind->Type == ViewType::SRV, "mismatched view");
				DestSlot = bi->NumCbvs + (bind.BindIndex - bi->SrvMin);
				SrcDesc = bind.ViewBind->SRVGfxState;
			}
			break;
		case BindType::Sampler:
		{
			sampler = true;
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = GetCPUDescriptor(&ctx->SamplerHeap,
					table->SamplerDescTableStart[frame] + bind.BindIndex - bi->SamplerMin);
				ctx->Device->CopyDescriptorsSimple(1, DestDesc, bind.SamplerBind->GfxState,
					D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			}
			break;
		}
		default:
			Unimplemented();
		}
		if (!sampler)
		{
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = GetCPUDescriptor(&ctx->CbvSrvUavHeap,
					table->CbvSrvUavDescTableStart[frame] + DestSlot);
				ctx->Device->CopyDescriptorsSimple(1, DestDesc, SrcDesc,
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
	}
}

void CopyConstantBufferDescriptors(gfx::Context* ctx, const gfx::DescriptorTable* table,
	const gfx::BindInfo* bi, const std::vector<ConstantBuffer>& CBs)
{
	for (const ConstantBuffer& buf : CBs)
	{
		u32 DestSlot = buf.Slot - bi->CbvMin;
		for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE DestDesc = GetCPUDescriptor(&ctx->CbvSrvUavHeap, 
				table->CbvSrvUavDescTableStart[frame] + DestSlot);
			ctx->Device->CopyDescriptorsSimple(1, DestDesc, 
				buf.GfxState.CbvDescriptor[frame],
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
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
			// TODO: do CBs need resource transitions?
			// cb.GfxState.State = D3D12_RESOURCE_STATE_GENERIC_READ;
			
			cb.GfxState.CbvDescriptor[frame] = gfx::AllocateDescriptor(
				&ctx->CbvSrvUavCreationHeap);

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

void AllocateDescriptorTables(gfx::Context* ctx, gfx::DescriptorTable* table,
	gfx::BindInfo* bi)
{
	for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
	{
		table->CbvSrvUavDescTableStart[frame] = ctx->CbvSrvUavHeap.NextIndex;
		ctx->CbvSrvUavHeap.NextIndex += (bi->NumCbvs + bi->NumSrvs + bi->NumUavs);
		table->SamplerDescTableStart[frame] = ctx->SamplerHeap.NextIndex;
		ctx->SamplerHeap.NextIndex += bi->NumSamplers;
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

		cs->GfxState.Blob = shaderBlob;

		CreateRootSignature(device, cs);

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.pRootSignature = cs->GfxState.RootSig;
		desc.CS.pShaderBytecode = shaderBlob->GetBufferPointer();
		desc.CS.BytecodeLength = shaderBlob->GetBufferSize();
		desc.NodeMask = 0;
		desc.CachedPSO.pCachedBlob = nullptr;
		desc.CachedPSO.CachedBlobSizeInBytes = 0;
		desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		hr = device->CreateComputePipelineState(&desc, __uuidof(ID3D12PipelineState),
			(void**)&cs->GfxState.Pipeline);
		Assert(hr == S_OK, "Failed to create pipeline state, hr=%x", hr);
	}

	for (VertexShader* vs : rd->VShaders)
	{
		ID3DBlob* shaderBlob = CommonCompileShader(&vs->Common, workingDirectory,
			"vs_5_0", errorState);

		HRESULT hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D12ShaderReflection, (void**) &vs->Common.Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		vs->GfxState.Blob = shaderBlob;

		GatherBinds(&vs->GfxState.BI, vs->Common.Reflector);
	}

	for (PixelShader* ps : rd->PShaders)
	{
		ID3DBlob* shaderBlob = CommonCompileShader(&ps->Common, workingDirectory,
			"ps_5_0", errorState);

		HRESULT hr = D3DReflect( shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
			IID_ID3D12ShaderReflection, (void**) &ps->Common.Reflector);
		Assert(hr == S_OK, "Failed to create reflection, hr=%x", hr);

		ps->GfxState.Blob = shaderBlob;
		GatherBinds(&ps->GfxState.BI, ps->Common.Reflector);
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

	for (Draw* draw : rd->Draws)
	{
		InitAssert(draw->VShader, "Null vertex shader on draw not permitted.");
		ID3D12ShaderReflection* reflector = draw->VShader->Common.Reflector;
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

		CreateRootSignature(device, draw);
		CreatePipelineState(device, draw);
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
		if (buf->ElementSizeExpr.IsValid() || buf->ElementCountExpr.IsValid())
		{
			ast::Result res;
			EvaluateExpression(evCtx, buf->ElementSizeExpr, res, UintType, 
				"Buffer::ElementSize");
			buf->ElementSize = res.Value.UintVal;
			EvaluateExpression(evCtx, buf->ElementCountExpr, res, UintType, 
				"Buffer::ElementCount");
			buf->ElementCount = res.Value.UintVal;
		}

		CreateBuffer(ctx, buf);
	}

	for (Texture* tex : rd->Textures)
	{
		if (tex->FromFile)
		{
			std::string filePath = dirPath + tex->FromFile;
			HANDLE file = fileio::OpenFileOptional(filePath.c_str(), GENERIC_READ);
			InitAssert(file != INVALID_HANDLE_VALUE, "Couldn't find DDS file: %s", 
				filePath.c_str());

			size_t extPos = filePath.rfind(".");
			InitAssert(extPos != std::string::npos, 
				"Could not find file extension in Texture::FromFile=%s \n"
				"	Only .tga and .dds are supported.",
				tex->FromFile);
			std::string ext = filePath.substr(extPos+1, 3);

			u32 ddsSize = fileio::GetFileSize(file);
			char* ddsBuffer = (char*)malloc(ddsSize);
			Assert(ddsBuffer != nullptr, "failed to alloc");

			fileio::ReadFile(file, ddsBuffer, ddsSize);

			CloseHandle(file);

			DirectX::TexMetadata meta = {};
			DirectX::ScratchImage scratch;
			if (ext == "dds")
				DirectX::LoadFromDDSMemory(ddsBuffer, ddsSize, DirectX::DDS_FLAGS_NONE, 
					&meta, scratch);
			else if (ext == "tga")
				DirectX::LoadFromTGAMemory(ddsBuffer, ddsSize, DirectX::TGA_FLAGS_NONE, 
					&meta, scratch);
			else
				InitError("Unsupported Texture::FromFile extension (%s)", ext.c_str());

			DXGI_FORMAT format = meta.format;
			tex->Size.x = (u32)meta.width;
			tex->Size.y = (u32)meta.height;
			Assert(meta.dimension == DirectX::TEX_DIMENSION_TEXTURE2D, "unsupported");
			D3D12_RESOURCE_DESC desc = {};
			desc.Format = format;
			desc.Width = (u32)meta.width;
			desc.Height = (u32)meta.height;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = (u16)meta.mipLevels;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Alignment = 0;

			D3D12_HEAP_PROPERTIES heap;
			heap.Type = D3D12_HEAP_TYPE_DEFAULT;
			heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heap.CreationNodeMask = 0;
			heap.VisibleNodeMask = 0;

			HRESULT hr = ctx->Device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, 
				&desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, 
				IID_PPV_ARGS(&tex->GfxState.Resource));
			Assert(hr == S_OK, "failed to create texture, hr=%x", hr);
			tex->GfxState.State = D3D12_RESOURCE_STATE_COPY_DEST;

			static u32 const MAX_TEXTURE_SUBRESOURCE_COUNT = 16;

			u64 textureMemorySize = 0;
			UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
			UINT64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];
			const u64 numSubResources = meta.mipLevels * meta.arraySize;
			Assert(numSubResources <= MAX_TEXTURE_SUBRESOURCE_COUNT, 
				"too many subresources.");
			 
			ctx->Device->GetCopyableFootprints(&desc, 0, (u32)numSubResources, 0, 
				layouts, numRows, rowSizesInBytes, &textureMemorySize);

			Assert(textureMemorySize < gfx::Context::UPLOAD_BUFFER_SIZE, 
				"upload data too large.");

			u8* uploadMemory = (u8*)ctx->UploadBufferMem;

			for (u64 arrayIndex = 0; arrayIndex < meta.arraySize; arrayIndex++)
			{
				for (u64 mipIndex = 0; mipIndex < meta.mipLevels; mipIndex++)
				{
					u64 sri = mipIndex + (arrayIndex * meta.mipLevels);
			 
					D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = layouts[sri];
					u64 subResourceHeight = numRows[sri];
					u64 subResourcePitch = AlignU32(subResourceLayout.Footprint.RowPitch, 
						D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
					u64 subResourceDepth = subResourceLayout.Footprint.Depth;
					u8* destinationSubResourceMemory = uploadMemory + subResourceLayout.Offset;
			 
					for (u64 sliceIndex = 0; sliceIndex < subResourceDepth; sliceIndex++)
					{
						const DirectX::Image* subImage = scratch.GetImage(mipIndex, 
							arrayIndex, sliceIndex);
						u8* sourceSubResourceMemory = subImage->pixels;
			 
						for (u64 height = 0; height < subResourceHeight; height++)
						{
							memcpy(destinationSubResourceMemory, sourceSubResourceMemory, 
								min(subResourcePitch, subImage->rowPitch));
							destinationSubResourceMemory += subResourcePitch;
							sourceSubResourceMemory += subImage->rowPitch;
						}
					}
				}
			}

			free(ddsBuffer); ddsBuffer = nullptr;

			gfx::Context::Frame* frameCtx = 
				&ctx->FrameContexts[ctx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT];
			ctx->UploadCommandList->Reset(frameCtx->CommandAllocator, nullptr);

			for (u64 sri = 0; sri < numSubResources; sri++)
			{
				D3D12_TEXTURE_COPY_LOCATION destination = {};
				destination.pResource = tex->GfxState.Resource;
				destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				destination.SubresourceIndex = (u8)sri;
			 
				D3D12_TEXTURE_COPY_LOCATION source = {};
				source.pResource = ctx->UploadBufferResource;
				source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				source.PlacedFootprint = layouts[sri];
				source.PlacedFootprint.Offset = 0;
			 
				ctx->UploadCommandList->CopyTextureRegion(&destination, 0, 0, 0,
					&source, nullptr);
			}

			ctx->UploadCommandList->Close();
			ctx->CommandQueue->ExecuteCommandLists(1, 
				(ID3D12CommandList* const*)&ctx->UploadCommandList);

			u64 fenceValue = ctx->FenceLastSignaledValue + 1;
			ctx->CommandQueue->Signal(ctx->Fence, fenceValue);
			ctx->FenceLastSignaledValue = fenceValue;

			frameCtx->FenceValue = 0;
			ctx->Fence->SetEventOnCompletion(fenceValue, ctx->FenceEvent);
			WaitForSingleObject(ctx->FenceEvent, INFINITE);
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

	for (Sampler* s : rd->Samplers)
	{
		D3D12_SAMPLER_DESC desc = {};
		desc.Filter = RlfToD3d(s->Filter);
		desc.AddressU = RlfToD3d(s->AddressMode.U);
		desc.AddressV = RlfToD3d(s->AddressMode.V);
		desc.AddressW = RlfToD3d(s->AddressMode.W);
		desc.MipLODBias = s->MipLODBias;
		desc.MaxAnisotropy = s->MaxAnisotropy;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor[0] = s->BorderColor.x;
		desc.BorderColor[1] = s->BorderColor.y;
		desc.BorderColor[2] = s->BorderColor.z;
		desc.BorderColor[3] = s->BorderColor.w;
		desc.MinLOD = s->MinLOD;
		desc.MaxLOD = s->MaxLOD;

		s->GfxState = gfx::AllocateDescriptor(&ctx->SamplerCreationHeap);
		device->CreateSampler(&desc, s->GfxState);
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		gfx::ComputeShader* cs = &dc->Shader->GfxState;
		AllocateDescriptorTables(ctx, &dc->GfxState.Table, &cs->BI);
		ApplyNullDescriptors(ctx, &cs->BI, &dc->GfxState.Table, dc->Shader->Common.Reflector);
		CopyBindDescriptors(ctx, resources, &cs->BI, &dc->GfxState.Table, dc->Binds);

		// Binding of constant buffers descriptors is separate from other views
		CopyConstantBufferDescriptors(ctx, &dc->GfxState.Table, &cs->BI, dc->CBs);

		if (dc->IndirectArgs)
		{
			D3D12_INDIRECT_ARGUMENT_DESC arg = {};
			arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &arg;
			desc.NodeMask = 0;
			HRESULT hr = ctx->Device->CreateCommandSignature(&desc, nullptr,
				IID_PPV_ARGS(&dc->GfxState.CommandSig));
			Assert(hr == S_OK, "Failed to create command signature, hr=%x", hr);
		}
	}

	for (Draw* d : rd->Draws)
	{
		gfx::VertexShader* vs = &d->VShader->GfxState;
		AllocateDescriptorTables(ctx, &d->GfxState.VSTable, &vs->BI);
		ApplyNullDescriptors(ctx, &vs->BI, &d->GfxState.VSTable, d->VShader->Common.Reflector);
		CopyBindDescriptors(ctx, resources, &vs->BI, &d->GfxState.VSTable, d->VSBinds);
		CopyConstantBufferDescriptors(ctx, &d->GfxState.VSTable, &vs->BI, d->VSCBs);
		if (d->PShader)
		{
			gfx::PixelShader* ps = &d->PShader->GfxState;
			AllocateDescriptorTables(ctx, &d->GfxState.PSTable, &ps->BI);
			ApplyNullDescriptors(ctx, &ps->BI, &d->GfxState.PSTable, d->PShader->Common.Reflector);
			CopyBindDescriptors(ctx, resources, &ps->BI, &d->GfxState.PSTable, d->PSBinds);
			CopyConstantBufferDescriptors(ctx, &d->GfxState.PSTable, &ps->BI, d->PSCBs);
		}

		Buffer* indirect_args = d->InstancedIndirectArgs ? d->InstancedIndirectArgs : 
			d->IndexedInstancedIndirectArgs;
		u32 indirec_args_size = d->InstancedIndirectArgs ? 
			sizeof(D3D12_DRAW_ARGUMENTS) : sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		D3D12_INDIRECT_ARGUMENT_TYPE type = d->InstancedIndirectArgs ?
			D3D12_INDIRECT_ARGUMENT_TYPE_DRAW : D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		if (indirect_args)
		{
			D3D12_INDIRECT_ARGUMENT_DESC arg = {};
			arg.Type = type;

			D3D12_COMMAND_SIGNATURE_DESC desc = {};
			desc.ByteStride = indirec_args_size;
			desc.NumArgumentDescs = 1;
			desc.pArgumentDescs = &arg;
			desc.NodeMask = 0;
			HRESULT hr = ctx->Device->CreateCommandSignature(&desc, nullptr,
				IID_PPV_ARGS(&d->GfxState.CommandSig));
			Assert(hr == S_OK, "Failed to create command signature, hr=%x", hr);
		}
	}

}

void ReleaseD3D(
	gfx::Context* ctx,
	RenderDescription* rd)
{
	// Reset descriptor heaps
	gfx::ResetHeap(&ctx->RtvHeap);
	gfx::ResetHeap(&ctx->DsvHeap);
	gfx::ResetHeap(&ctx->CbvSrvUavCreationHeap);
	gfx::ResetHeap(&ctx->CbvSrvUavHeap);
	gfx::ResetHeap(&ctx->SamplerCreationHeap);
	gfx::ResetHeap(&ctx->SamplerHeap);

	for (ComputeShader* cs : rd->CShaders)
	{
		SafeRelease(cs->GfxState.Pipeline);
		SafeRelease(cs->GfxState.RootSig);
		SafeRelease(cs->Common.Reflector);
		SafeRelease(cs->GfxState.Blob);
	}

	for (VertexShader* vs : rd->VShaders)
	{
		SafeRelease(vs->Common.Reflector);
		SafeRelease(vs->GfxState.Blob);
	}
	for (PixelShader* ps : rd->PShaders)
	{
		SafeRelease(ps->Common.Reflector);
		SafeRelease(ps->GfxState.Blob);
	}

	for (Draw* d : rd->Draws)
	{
		SafeRelease(d->GfxState.CommandSig);
		SafeRelease(d->GfxState.Pipeline);
		SafeRelease(d->GfxState.RootSig);
		for (ConstantBuffer& cb : d->VSCBs)
		{
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				cb.GfxState.Resource[frame]->Unmap(0, nullptr);
				SafeRelease(cb.GfxState.Resource[frame]);
			}
			free(cb.BackingMemory);
		}
		for (ConstantBuffer& cb : d->PSCBs)
		{
			for (u32 frame = 0 ; frame < gfx::Context::NUM_FRAMES_IN_FLIGHT ; ++frame)
			{
				cb.GfxState.Resource[frame]->Unmap(0, nullptr);
				SafeRelease(cb.GfxState.Resource[frame]);
			}
			free(cb.BackingMemory);
		}
	}

	for (Dispatch* dc : rd->Dispatches)
	{
		SafeRelease(dc->GfxState.CommandSig);
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
			if (tex->FromFile)
				continue;
			if ((tex->SizeExpr.Dep.VariesByFlags & ec->EvCtx.ChangedThisFrameFlags) == 0)
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
			if (!buf->ElementSizeExpr.IsValid() && !buf->ElementCountExpr.IsValid())
				continue;

			if ((buf->ElementSizeExpr.Dep.VariesByFlags & ec->EvCtx.ChangedThisFrameFlags) == 0 &&
				(buf->ElementCountExpr.Dep.VariesByFlags & ec->EvCtx.ChangedThisFrameFlags) == 0)
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
				
				CreateBuffer(ctx, buf);

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

	for (Dispatch* dc : rd->Dispatches)
	{
		CopyBindDescriptors(ctx, &ec->Res, &dc->Shader->GfxState.BI, 
			&dc->GfxState.Table, dc->Binds);
	}
	for (Draw* d : rd->Draws)
	{
		CopyBindDescriptors(ctx, &ec->Res, &d->VShader->GfxState.BI, 
			&d->GfxState.VSTable, d->VSBinds);
		if (d->PShader)
			CopyBindDescriptors(ctx, &ec->Res, &d->PShader->GfxState.BI, 
				&d->GfxState.PSTable, d->PSBinds);
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
	for (SetConstant& set : sets)
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
	for (ConstantBuffer& buf : buffers)
	{
		u32 frame = ctx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT;
		memcpy(buf.GfxState.MappedMem[frame], buf.BackingMemory, buf.Size);
	}
}

void TransitionBinds(gfx::Context* ctx, ExecuteResources* resources, std::vector<Bind>& binds)
{
	for (const Bind& bind : binds)
	{
		switch(bind.Type)
		{
		case BindType::SystemValue:
			switch (bind.SystemBind)
			{
			case SystemValue::BackBuffer:
				if (bind.IsOutput)
				{
					TransitionResource(ctx, resources->MainRtTex, 
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
				if (bind.ViewBind->ResourceType == ResourceType::Buffer)
					TransitionResource(ctx, &bind.ViewBind->Buffer->GfxState,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				else if (bind.ViewBind->ResourceType == ResourceType::Texture)
					TransitionResource(ctx, &bind.ViewBind->Texture->GfxState,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
			else
			{
				Assert(bind.ViewBind->Type == ViewType::SRV, "mismatched view");
				if (bind.ViewBind->ResourceType == ResourceType::Buffer)
					TransitionResource(ctx, &bind.ViewBind->Buffer->GfxState,
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | 
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				else if (bind.ViewBind->ResourceType == ResourceType::Texture)
					TransitionResource(ctx, &bind.ViewBind->Texture->GfxState,
						D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | 
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
			break;
		case BindType::Sampler:
			break;
		default:
			Unimplemented();
		}
	}
}

void ExecuteDispatch(
	Dispatch* dc,
	ExecuteContext* ec)
{
	gfx::ComputeShader* cs = &dc->Shader->GfxState;
	ExecuteSetConstants(ec, dc->Constants, dc->CBs);

	TransitionBinds(ec->GfxCtx, &ec->Res, dc->Binds);

	u32 frame = ec->GfxCtx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT;

	ID3D12GraphicsCommandList* cl = ec->GfxCtx->CommandList;
	cl->SetPipelineState(cs->Pipeline);
	cl->SetComputeRootSignature(cs->RootSig);
	u32 table_index = 0;
	if (cs->BI.NumCbvs + cs->BI.NumSrvs + cs->BI.NumUavs > 0)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE cbv_srv_uav_table_handle = GetGPUDescriptor(
			&ec->GfxCtx->CbvSrvUavHeap, dc->GfxState.Table.CbvSrvUavDescTableStart[frame]);
		cl->SetComputeRootDescriptorTable(table_index++, cbv_srv_uav_table_handle);
	}
	if (cs->BI.NumSamplers > 0)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE sampler_table_handle = GetGPUDescriptor(
			&ec->GfxCtx->SamplerHeap, dc->GfxState.Table.SamplerDescTableStart[frame]);
		cl->SetComputeRootDescriptorTable(table_index++, sampler_table_handle);
	}

	if (dc->IndirectArgs)
	{
		TransitionResource(ec->GfxCtx, &dc->IndirectArgs->GfxState,
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		cl->ExecuteIndirect(dc->GfxState.CommandSig, 1, dc->IndirectArgs->GfxState.Resource, 
			dc->IndirectArgsOffset, nullptr, 0);
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
		else if (dc->Groups.IsValid())
		{
			ast::Result res;
			EvaluateExpression(ec->EvCtx, dc->Groups, res, Uint3Type, "Dispatch::Groups");
			groups = res.Value.Uint3Val;
		}

		cl->Dispatch(groups.x, groups.y, groups.z);
	}
}

void ExecuteDraw(
	Draw* draw,
	ExecuteContext* ec)
{
	ID3D12GraphicsCommandList* cl = ec->GfxCtx->CommandList;
	ExecuteSetConstants(ec, draw->VSConstants, draw->VSCBs);
	ExecuteSetConstants(ec, draw->PSConstants, draw->PSCBs);

	TransitionBinds(ec->GfxCtx, &ec->Res, draw->VSBinds);
	TransitionBinds(ec->GfxCtx, &ec->Res, draw->PSBinds);

	u32 frame = ec->GfxCtx->FrameIndex % gfx::Context::NUM_FRAMES_IN_FLIGHT;

	cl->SetGraphicsRootSignature(draw->GfxState.RootSig);
	cl->SetPipelineState(draw->GfxState.Pipeline);

	gfx::BindInfo& vsbi = draw->VShader->GfxState.BI;

	u32 table_index = 0;
	if (vsbi.NumCbvs + vsbi.NumSrvs + vsbi.NumUavs > 0)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE cbv_srv_uav_table_handle = GetGPUDescriptor(
			&ec->GfxCtx->CbvSrvUavHeap, draw->GfxState.VSTable.CbvSrvUavDescTableStart[frame]);
		cl->SetGraphicsRootDescriptorTable(table_index++, cbv_srv_uav_table_handle);
	}
	if (vsbi.NumSamplers > 0)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE sampler_table_handle = GetGPUDescriptor(
			&ec->GfxCtx->SamplerHeap, draw->GfxState.VSTable.SamplerDescTableStart[frame]);
		cl->SetGraphicsRootDescriptorTable(table_index++, sampler_table_handle);
	}
	if (draw->PShader)
	{
		gfx::BindInfo& psbi = draw->PShader->GfxState.BI;
		if (psbi.NumCbvs + psbi.NumSrvs + psbi.NumUavs > 0)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE cbv_srv_uav_table_handle = GetGPUDescriptor(
				&ec->GfxCtx->CbvSrvUavHeap, draw->GfxState.PSTable.CbvSrvUavDescTableStart[frame]);
			cl->SetGraphicsRootDescriptorTable(table_index++, cbv_srv_uav_table_handle);
		}
		if (psbi.NumSamplers > 0)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE sampler_table_handle = GetGPUDescriptor(
				&ec->GfxCtx->SamplerHeap, draw->GfxState.PSTable.SamplerDescTableStart[frame]);
			cl->SetGraphicsRootDescriptorTable(table_index++, sampler_table_handle);
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtViews[8] = {};
	D3D12_VIEWPORT vp[8] = {};
	u32 rtCount = 0;
	for (TextureTarget target : draw->RenderTargets)
	{
		gfx::Texture* resource = nullptr;
		if (target.IsSystem)
		{
			if (target.System == SystemValue::BackBuffer)
			{
				resource = ec->Res.MainRtTex;
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
			resource = &target.View->Texture->GfxState;
			rtViews[rtCount] = target.View->RTVGfxState;
			vp[rtCount].Width = (float)target.View->Texture->Size.x;
			vp[rtCount].Height = (float)target.View->Texture->Size.y;
		}
		vp[rtCount].MinDepth = 0.0f;
		vp[rtCount].MaxDepth = 1.0f;
		vp[rtCount].TopLeftX = vp[rtCount].TopLeftY = 0;
		++rtCount;

		TransitionResource(ec->GfxCtx, resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	D3D12_CPU_DESCRIPTOR_HANDLE dsView = {};
	for (TextureTarget target : draw->DepthStencil)
	{
		gfx::Texture* resource = nullptr;
		if (target.IsSystem)
		{
			if (target.System == SystemValue::DefaultDepth)
			{
				resource = ec->Res.DefaultDepthTex;
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
			resource = &target.View->Texture->GfxState;
			dsView = target.View->DSVGfxState;
			vp[0].Width = (float)target.View->Texture->Size.x;
			vp[0].Height = (float)target.View->Texture->Size.y;
		}
		vp[0].MinDepth = 0.0f;
		vp[0].MaxDepth = 1.0f;

		// TODO: read-only state should be set based on usage
		TransitionResource(ec->GfxCtx, resource, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	for (int i = 0 ; i < draw->Viewports.size() ; ++i)
	{
		Viewport* v = draw->Viewports[i];
		ast::Result res;
		if (v->TopLeft.IsValid()) {
			EvaluateExpression(ec->EvCtx, v->TopLeft, res, Float2Type, "Viewport::TopLeft");
			vp[i].TopLeftX = res.Value.Float2Val.x;
			vp[i].TopLeftY = res.Value.Float2Val.y;
		}
		if (v->Size.IsValid()) {
			EvaluateExpression(ec->EvCtx, v->Size, res, Float2Type, "Viewport::Size");
			vp[i].Width = res.Value.Float2Val.x;
			vp[i].Height = res.Value.Float2Val.y;
		}
		if (v->DepthRange.IsValid()) {
			EvaluateExpression(ec->EvCtx, v->DepthRange, res, Float2Type, "Viewport::DepthRange");
			vp[i].MinDepth = res.Value.Float2Val.x;
			vp[i].MaxDepth = res.Value.Float2Val.y;
		}
	}
	u32 vpCount = draw->DepthStencil.size() > 0 ? 1 : 0;
	vpCount = max(vpCount, rtCount);
	D3D12_RECT sr[8] = {};
	for (u32 i = 0 ; i < vpCount ; ++i)
	{
		sr[i].left = (u32)vp[i].TopLeftX;
		sr[i].top = (u32)vp[i].TopLeftY;
		sr[i].right = (u32)(vp[i].TopLeftX + vp[i].Width);
		sr[i].bottom = (u32)(vp[i].TopLeftY + vp[i].Height);
	}
	cl->OMSetRenderTargets(rtCount, rtViews, false, 
		draw->DepthStencil.size() > 0 ? &dsView : nullptr);
	cl->RSSetViewports(vpCount, vp);
	cl->RSSetScissorRects(vpCount, sr);
	cl->IASetPrimitiveTopology(RlfToD3d_Topo(draw->Topology));
	cl->OMSetStencilRef(draw->StencilRef);
	if (!draw->VertexBuffers.empty())
	{
		D3D12_VERTEX_BUFFER_VIEW vbv[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
		for (u32 i = 0 ; i < draw->VertexBuffers.size() ; ++i)
		{
			Buffer* vb = draw->VertexBuffers[i];
			TransitionResource(ec->GfxCtx, &vb->GfxState, 
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			vbv[i].BufferLocation = vb->GfxState.Resource->GetGPUVirtualAddress();
			vbv[i].SizeInBytes = vb->ElementCount * vb->ElementSize;
			vbv[i].StrideInBytes = vb->ElementSize;
		}

		cl->IASetVertexBuffers(0, (u32)draw->VertexBuffers.size(), &vbv[0]);
	}
	Buffer* ib = draw->IndexBuffer;
	if (ib)
	{
		TransitionResource(ec->GfxCtx, &ib->GfxState, 
			D3D12_RESOURCE_STATE_INDEX_BUFFER);

		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.BufferLocation = ib->GfxState.Resource->GetGPUVirtualAddress();
		ibv.SizeInBytes = ib->ElementCount * ib->ElementSize;
		ibv.Format = ib->ElementSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		cl->IASetIndexBuffer(&ibv);
	}

	if (draw->InstancedIndirectArgs)
	{
		TransitionResource(ec->GfxCtx, &draw->InstancedIndirectArgs->GfxState,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		cl->ExecuteIndirect(draw->GfxState.CommandSig, 1, 
			draw->InstancedIndirectArgs->GfxState.Resource, 
			draw->IndirectArgsOffset, nullptr, 0);
	}
	else if (draw->IndexedInstancedIndirectArgs)
	{
		TransitionResource(ec->GfxCtx, &draw->IndexedInstancedIndirectArgs->GfxState,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		cl->ExecuteIndirect(draw->GfxState.CommandSig, 1, 
			draw->IndexedInstancedIndirectArgs->GfxState.Resource, 
			draw->IndirectArgsOffset, nullptr, 0);
	}
	else if (ib && draw->InstanceCount > 0)
		cl->DrawIndexedInstanced(ib->ElementCount, draw->InstanceCount, 0, 0, 0);
	else if (ib)
		cl->DrawIndexedInstanced(ib->ElementCount, 1, 0, 0, 0);
	else if (draw->InstanceCount > 0)
		cl->DrawInstanced(draw->VertexCount, draw->InstanceCount, 0, 0);
	else
		cl->DrawInstanced(draw->VertexCount, 1, 0, 0);
}

void _Execute(
	ExecuteContext* ec,
	RenderDescription* rd)
{
	gfx::Context* ctx = ec->GfxCtx;

	EvaluateConstants(ec->EvCtx, rd->Constants);

	// Clear state so we aren't polluted by previous program drawing or previous 
	//	execution. 
	ctx->CommandList->ClearState(nullptr);
	ID3D12DescriptorHeap* ShaderDescriptorHeaps[2] = {
		ctx->CbvSrvUavHeap.Object, ctx->SamplerHeap.Object
	};
	ctx->CommandList->SetDescriptorHeaps(2, ShaderDescriptorHeaps);


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
			TransitionResource(ctx, &pass.ClearColor->Target->Texture->GfxState,
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			float4& color = pass.ClearColor->Color;
			const float clear_color[4] =
			{
				color.x, color.y, color.z, color.w
			};
			ctx->CommandList->ClearRenderTargetView(pass.ClearColor->Target->RTVGfxState, 
				clear_color, 0, nullptr);
		}
		else if (pass.Type == PassType::ClearDepth)
		{
			TransitionResource(ctx, &pass.ClearDepth->Target->Texture->GfxState,
				D3D12_RESOURCE_STATE_DEPTH_WRITE);
			ctx->CommandList->ClearDepthStencilView(pass.ClearDepth->Target->DSVGfxState, 
				D3D12_CLEAR_FLAG_DEPTH, pass.ClearDepth->Depth, 0, 0, nullptr);
		}
		else if (pass.Type == PassType::ClearStencil)
		{
			TransitionResource(ctx, &pass.ClearStencil->Target->Texture->GfxState,
				D3D12_RESOURCE_STATE_DEPTH_WRITE);
			ctx->CommandList->ClearDepthStencilView(pass.ClearStencil->Target->DSVGfxState, 
				D3D12_CLEAR_FLAG_STENCIL, 0.f, pass.ClearStencil->Stencil, 0, nullptr);
		}
		else if (pass.Type == PassType::Resolve)
		{
			TransitionResource(ctx, &pass.Resolve->Src->GfxState,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
			TransitionResource(ctx, &pass.Resolve->Dst->GfxState,
				D3D12_RESOURCE_STATE_RESOLVE_DEST);
			ctx->CommandList->ResolveSubresource(pass.Resolve->Dst->GfxState.Resource, 0, 
				pass.Resolve->Src->GfxState.Resource, 0, 
				D3DTextureFormat[(u32)pass.Resolve->Dst->Format]);
		}
		else if (pass.Type == PassType::ObjDraw)
		{
			for (Draw* draw : pass.ObjDraw->PerMeshDraws)
			{
				ExecuteDraw(draw, ec);
			}
		}
		else
		{
			Unimplemented();
		}

		// Clear state after execution so we don't pollute the rest of program drawing. 
		ctx->CommandList->ClearState(nullptr);
		ctx->CommandList->SetDescriptorHeaps(2, ShaderDescriptorHeaps);
	}
}

#undef ExecuteAssert
#undef SafeRelease

} // namespace rlf
